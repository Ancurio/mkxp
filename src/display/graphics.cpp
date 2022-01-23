/*
 ** graphics.cpp
 **
 ** This file is part of mkxp.
 **
 ** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
 **
 ** mkxp is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** mkxp is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "graphics.h"

#include "audio.h"
#include "binding.h"
#include "bitmap.h"
#include "config.h"
#include "debugwriter.h"
#include "disposable.h"
#include "etc-internal.h"
#include "eventthread.h"
#include "filesystem.h"
#include "gl-fun.h"
#include "gl-util.h"
#include "glstate.h"
#include "intrulist.h"
#include "quad.h"
#include "scene.h"
#include "shader.h"
#include "sharedstate.h"
#include "texpool.h"
#include "theoraplay/theoraplay.h"
#include "util.h"
#include "input.h"
#include "sprite.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_timer.h>
#include <SDL_video.h>
#include <SDL_mutex.h>
#include <SDL_thread.h>

#ifdef MKXPZ_STEAM
#include "steamshim_child.h"
#endif

#include <algorithm>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <cmath>


#define DEF_SCREEN_W (rgssVer == 1 ? 640 : 544)
#define DEF_SCREEN_H (rgssVer == 1 ? 480 : 416)

#define DEF_FRAMERATE (rgssVer == 1 ? 40 : 60)

#define DEF_MAX_VIDEO_FRAMES 30
#define VIDEO_DELAY 10
#define AUDIO_DELAY 100

typedef struct AudioQueue
{
    const THEORAPLAY_AudioPacket *audio;
    int offset;
    struct AudioQueue *next;
} AudioQueue;

static volatile AudioQueue *movieAudioQueue;
static volatile AudioQueue *movieAudioQueueTail;


static long readMovie(THEORAPLAY_Io *io, void *buf, long buflen)
{
    SDL_RWops *f = (SDL_RWops *) io->userdata;
    return (long) SDL_RWread(f, buf, 1, buflen);
} // IoFopenRead


static void closeMovie(THEORAPLAY_Io *io)
{
    SDL_RWops *f = (SDL_RWops *) io->userdata;
    SDL_RWclose(f);
    free(io);
} // IoFopenClose


struct Movie
{
    THEORAPLAY_Decoder *decoder;
    const THEORAPLAY_AudioPacket *audio;
    const THEORAPLAY_VideoFrame *video;
    bool hasVideo;
    bool hasAudio;
    bool skippable;
    Bitmap *videoBitmap;
    SDL_RWops srcOps;    
    static float volume;

    Movie(int volume_, bool skippable_)
    : decoder(0), audio(0), video(0), skippable(skippable_), videoBitmap(0)
    {
        volume = volume_ * 0.01f;
    }
    bool preparePlayback()
    {

        // https://theora.org/doc/libtheora-1.0/codec_8h.html
        // https://ffmpeg.org/doxygen/0.11/group__lavc__misc__pixfmt.html
        THEORAPLAY_Io *io = (THEORAPLAY_Io *) malloc(sizeof (THEORAPLAY_Io));        
        if(!io) {
            SDL_RWclose(&srcOps);
            return false;
        }
        
        io->read = readMovie;
        io->close = closeMovie;
        io->userdata = &srcOps;
        decoder = THEORAPLAY_startDecode(io, DEF_MAX_VIDEO_FRAMES, THEORAPLAY_VIDFMT_RGBA);
        if (!decoder) {
            SDL_RWclose(&srcOps);
            return false;
        }

        // Wait until the decoder has parsed out some basic truths from the file.
        while (!THEORAPLAY_isInitialized(decoder)) {
            SDL_Delay(VIDEO_DELAY);
        }

        // Once we're initialized, we can tell if this file has audio and/or video.
        hasAudio = THEORAPLAY_hasAudioStream(decoder);
        hasVideo = THEORAPLAY_hasVideoStream(decoder);

        // Queue up the audio
        if (hasAudio) {
            while ((audio = THEORAPLAY_getAudio(decoder)) == NULL) {
                if ((THEORAPLAY_availableVideo(decoder) >= DEF_MAX_VIDEO_FRAMES)) {
                    break;  // we'll never progress, there's no audio yet but we've prebuffered as much as we plan to.
                }
                SDL_Delay(VIDEO_DELAY);
            }
        }

        // No video, so no point in doing anything else
        if (!hasVideo) {
            THEORAPLAY_stopDecode(decoder);
            return false;
        }

        // Wait until we have video
        while ((video = THEORAPLAY_getVideo(decoder)) == NULL) {
            SDL_Delay(VIDEO_DELAY);
        }

        // Wait until we have audio, if applicable
        audio = NULL;
        if (hasAudio) {
            while ((audio = THEORAPLAY_getAudio(decoder)) == NULL && THEORAPLAY_availableVideo(decoder) < DEF_MAX_VIDEO_FRAMES) {
                SDL_Delay(VIDEO_DELAY);
            }
        }
        videoBitmap = new Bitmap(video->width, video->height);
        movieAudioQueue = NULL;
        movieAudioQueueTail = NULL;

        return true;
    }

    static void queueAudioPacket(const THEORAPLAY_AudioPacket *audio) {
        AudioQueue *item = NULL;

        if (!audio) {
            return;
        }

        item = (AudioQueue *) malloc(sizeof (AudioQueue));
        if (!item) {
            THEORAPLAY_freeAudio(audio);
            return;  // oh well.
        }

        item->audio = audio;
        item->offset = 0;
        item->next = NULL;


        SDL_LockAudio();
        if (movieAudioQueueTail) {
            movieAudioQueueTail->next = item;
        } else {
            movieAudioQueue = item;
        }
        movieAudioQueueTail = item;
        SDL_UnlockAudio();
    }

    static void queueMoreMovieAudio(THEORAPLAY_Decoder *decoder, const Uint32 now) {
        const THEORAPLAY_AudioPacket *audio;
        while ((audio = THEORAPLAY_getAudio(decoder)) != NULL) {
            queueAudioPacket(audio);
            if (audio->playms >= now + 2000) {  // don't let this get too far ahead.
                break;
            }
        }
    }
    
    static void SDLCALL movieAudioCallback(void *userdata, uint8_t *stream, int len) {
        // !!! FIXME: this should refuse to play if item->playms is in the future.
        //const Uint32 now = SDL_GetTicks() - baseticks;
        Sint16 *dst = (Sint16 *) stream;

        while (movieAudioQueue && (len > 0)) {
            volatile AudioQueue *item = movieAudioQueue;
            AudioQueue *next = item->next;
            const int channels = item->audio->channels;

            const float *src = item->audio->samples + (item->offset * channels);
            unsigned int cpy = (item->audio->frames - item->offset) * channels;
            int i;

            if (cpy > (len / sizeof (Sint16))) {
                cpy = len / sizeof (Sint16);
            }

            for (i = 0; i < cpy; i++) {
                const float val = (*(src++)) * volume;
                if (val < -1.0f) {
                    *(dst++) = -32768;
                } else if (val > 1.0f) {
                    *(dst++) = 32767;
                } else {
                    *(dst++) = (Sint16) (val * 32767.0f);
                }
            }

            item->offset += (cpy / channels);
            len -= cpy * sizeof (Sint16);

            if (item->offset >= item->audio->frames) {
                THEORAPLAY_freeAudio(item->audio);
                free((void *) item);
                movieAudioQueue = next;
            }
        }

        if (!movieAudioQueue) {
            movieAudioQueueTail = NULL;
        }

        if (len > 0) {
            memset(dst, '\0', len);
        }
    }

    bool startAudio()
    {
        SDL_AudioSpec spec{};
        spec.freq = audio->freq;
        spec.format = AUDIO_S16SYS;
        spec.channels = audio->channels;
        spec.samples = 2048;
        spec.callback = movieAudioCallback;
        if (SDL_OpenAudio(&spec, NULL) != 0) {
            return false;
        }
        queueAudioPacket(audio);
        audio = NULL;
        queueMoreMovieAudio(decoder, 0);
        SDL_PauseAudio(0);  // Start audio playback
        return true;
    }

    void play()
    {
        // Assuming every frame has the same duration.
        // Uint32 frameMs = (video->fps == 0.0) ? 0 : ((Uint32) (1000.0 / video->fps));
        Uint32 baseTicks = SDL_GetTicks();
        bool openedAudio = false;
        while (THEORAPLAY_isDecoding(decoder)) {
            // Check for reset/shutdown input
            if(shState->graphics().updateMovieInput(this)) break;
            
            // Check for attempted skip
            if (skippable) {
                shState->input().update();
                if  (shState->input().isTriggered(Input::C) || shState->input().isTriggered(Input::B)) break;
            }

            const Uint32 now = SDL_GetTicks() - baseTicks;

            if (!video) {
                video = THEORAPLAY_getVideo(decoder);
            }

            if (hasAudio) {
                if (!audio) {
                    audio = THEORAPLAY_getAudio(decoder);
                }

                if (audio && !openedAudio) {
                    if(!startAudio()){
                        Debug() << "Error opening movie audio!";
                        break;
                    }
                    openedAudio = true;
                }

            }

            // Got a video frame, now draw it
            if (video) {
                videoBitmap->replaceRaw(video->pixels, video->width * video->height * 4);
                THEORAPLAY_freeVideo(video);
                video = NULL;
            }
            shState->graphics().update(false);
        
            if (openedAudio) {
                queueMoreMovieAudio(decoder, now);
            }
        }
    }

    ~Movie()
    {
        if (hasAudio) {
            if (movieAudioQueueTail) {
                THEORAPLAY_freeAudio(movieAudioQueueTail->audio);
            }
            movieAudioQueueTail = NULL;

            if (movieAudioQueue) {
                THEORAPLAY_freeAudio(movieAudioQueue->audio);
            }
            movieAudioQueue = NULL;
        }
        if (video) THEORAPLAY_freeVideo(video);
        if (audio) THEORAPLAY_freeAudio(audio);
        if (decoder) THEORAPLAY_stopDecode(decoder);
        SDL_CloseAudio();
        delete videoBitmap;
    }
};

float Movie::volume;

struct MovieOpenHandler : FileSystem::OpenHandler
{
    SDL_RWops *srcOps;

    MovieOpenHandler(SDL_RWops &srcOps)
    :   srcOps(&srcOps)
    {}
    
    bool tryRead(SDL_RWops &ops, const char *ext)
    {
        *srcOps = ops;
        return true;
    }
};

struct PingPong {
    TEXFBO rt[2];
    uint8_t srcInd, dstInd;
    int screenW, screenH;
    
    PingPong(int screenW, int screenH)
    : srcInd(0), dstInd(1), screenW(screenW), screenH(screenH) {
        for (int i = 0; i < 2; ++i) {
            TEXFBO::init(rt[i]);
            TEXFBO::allocEmpty(rt[i], screenW, screenH);
            TEXFBO::linkFBO(rt[i]);
            gl.ClearColor(0, 0, 0, 1);
            FBO::clear();
        }
    }
    
    ~PingPong() {
        for (int i = 0; i < 2; ++i)
        TEXFBO::fini(rt[i]);
    }
    
    TEXFBO &backBuffer() { return rt[srcInd]; }
    
    TEXFBO &frontBuffer() { return rt[dstInd]; }
    
    /* Better not call this during render cycles */
    void resize(int width, int height) {
        screenW = width;
        screenH = height;
        
        for (int i = 0; i < 2; ++i)
        TEXFBO::allocEmpty(rt[i], width, height);
    }
    
    void startRender() { bind(); }
    
    void swapRender() {
        std::swap(srcInd, dstInd);
        
        bind();
    }
    
    void clearBuffers() {
        glState.clearColor.pushSet(Vec4(0, 0, 0, 1));
        
        for (int i = 0; i < 2; ++i) {
            FBO::bind(rt[i].fbo);
            FBO::clear();
        }
        
        glState.clearColor.pop();
    }
    
private:
    void bind() { FBO::bind(rt[dstInd].fbo); }
};

class ScreenScene : public Scene {
public:
    ScreenScene(int width, int height) : pp(width, height) {
        updateReso(width, height);
        
        brightEffect = false;
        brightnessQuad.setColor(Vec4());
    }
    
    void composite() {
        const int w = geometry.rect.w;
        const int h = geometry.rect.h;
        
        shState->prepareDraw();
        
        pp.startRender();
        
        glState.viewport.set(IntRect(0, 0, w, h));
        
        FBO::clear();
        
        Scene::composite();
        
        if (brightEffect) {
            SimpleColorShader &shader = shState->shaders().simpleColor;
            shader.bind();
            shader.applyViewportProj();
            shader.setTranslation(Vec2i());
            
            brightnessQuad.draw();
        }
    }
    
    void requestViewportRender(const Vec4 &c, const Vec4 &f, const Vec4 &t) {
        const IntRect &viewpRect = glState.scissorBox.get();
        const IntRect &screenRect = geometry.rect;
        
        const bool toneRGBEffect = t.xyzNotNull();
        const bool toneGrayEffect = t.w != 0;
        const bool colorEffect = c.w > 0;
        const bool flashEffect = f.w > 0;
        
        if (toneGrayEffect) {
            pp.swapRender();
            
            if (!viewpRect.encloses(screenRect)) {
                /* Scissor test _does_ affect FBO blit operations,
                 * and since we're inside the draw cycle, it will
                 * be turned on, so turn it off temporarily */
                glState.scissorTest.pushSet(false);
                
                GLMeta::blitBegin(pp.frontBuffer());
                GLMeta::blitSource(pp.backBuffer());
                GLMeta::blitRectangle(geometry.rect, Vec2i());
                GLMeta::blitEnd();
                
                glState.scissorTest.pop();
            }
            
            GrayShader &shader = shState->shaders().gray;
            shader.bind();
            shader.setGray(t.w);
            shader.applyViewportProj();
            shader.setTexSize(screenRect.size());
            
            TEX::bind(pp.backBuffer().tex);
            
            glState.blend.pushSet(false);
            screenQuad.draw();
            glState.blend.pop();
        }
        
        if (!toneRGBEffect && !colorEffect && !flashEffect)
            return;
        
        FlatColorShader &shader = shState->shaders().flatColor;
        shader.bind();
        shader.applyViewportProj();
        
        if (toneRGBEffect) {
            /* First split up additive / substractive components */
            Vec4 add, sub;
            
            if (t.x > 0)
                add.x = t.x;
            if (t.y > 0)
                add.y = t.y;
            if (t.z > 0)
                add.z = t.z;
            
            if (t.x < 0)
                sub.x = -t.x;
            if (t.y < 0)
                sub.y = -t.y;
            if (t.z < 0)
                sub.z = -t.z;
            
            /* Then apply them using hardware blending */
            gl.BlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
            
            if (add.xyzNotNull()) {
                gl.BlendEquation(GL_FUNC_ADD);
                shader.setColor(add);
                
                screenQuad.draw();
            }
            
            if (sub.xyzNotNull()) {
                gl.BlendEquation(GL_FUNC_REVERSE_SUBTRACT);
                shader.setColor(sub);
                
                screenQuad.draw();
            }
        }
        
        if (colorEffect || flashEffect) {
            gl.BlendEquation(GL_FUNC_ADD);
            gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO,
                                 GL_ONE);
        }
        
        if (colorEffect) {
            shader.setColor(c);
            screenQuad.draw();
        }
        
        if (flashEffect) {
            shader.setColor(f);
            screenQuad.draw();
        }
        
        glState.blendMode.refresh();
    }
    
    void setBrightness(float norm) {
        brightnessQuad.setColor(Vec4(0, 0, 0, 1.0f - norm));
        
        brightEffect = norm < 1.0f;
    }
    
    void updateReso(int width, int height) {
        geometry.rect.w = width;
        geometry.rect.h = height;
        
        screenQuad.setTexPosRect(geometry.rect, geometry.rect);
        brightnessQuad.setTexPosRect(geometry.rect, geometry.rect);
        
        notifyGeometryChange();
    }
    
    void setResolution(int width, int height) {
        pp.resize(width, height);
        updateReso(width, height);
    }
    
    PingPong &getPP() { return pp; }
    
private:
    PingPong pp;
    Quad screenQuad;
    
    Quad brightnessQuad;
    bool brightEffect;
};

/* Nanoseconds per second */
#define NS_PER_S 1000000000

struct FPSLimiter {
    uint64_t lastTickCount;
    
    /* ticks per frame */
    int64_t tpf;
    
    /* Ticks per second */
    const uint64_t tickFreq;
    
    /* Ticks per milisecond */
    const uint64_t tickFreqMS;
    
    /* Ticks per nanosecond */
    const double tickFreqNS;
    
    bool disabled;
    
    /* Data for frame timing adjustment */
    struct {
        /* Last tick count */
        uint64_t last;
        
        /* How far behind/in front we are for ideal frame timing */
        int64_t idealDiff;
        
        bool resetFlag;
    } adj;
    
    FPSLimiter(uint16_t desiredFPS)
    : lastTickCount(SDL_GetPerformanceCounter()),
    tickFreq(SDL_GetPerformanceFrequency()), tickFreqMS(tickFreq / 1000),
    tickFreqNS((double)tickFreq / NS_PER_S), disabled(false) {
        setDesiredFPS(desiredFPS);
        
        adj.last = SDL_GetPerformanceCounter();
        adj.idealDiff = 0;
        adj.resetFlag = false;
    }
    
    void setDesiredFPS(uint16_t value) { tpf = tickFreq / value; }
    
    void delay() {
        if (disabled)
            return;
        
        int64_t tickDelta = SDL_GetPerformanceCounter() - lastTickCount;
        int64_t toDelay = tpf - tickDelta;
        
        /* Compensate for the last delta
         * to the ideal timestep */
        toDelay -= adj.idealDiff;
        
        if (toDelay < 0)
            toDelay = 0;
        
        delayTicks(toDelay);
        
        uint64_t now = lastTickCount = SDL_GetPerformanceCounter();
        int64_t diff = now - adj.last;
        adj.last = now;
        
        /* Recalculate our temporal position
         * relative to the ideal timestep */
        adj.idealDiff = diff - tpf + adj.idealDiff;
        
        if (adj.resetFlag) {
            adj.idealDiff = 0;
            adj.resetFlag = false;
        }
    }
    
    void resetFrameAdjust() { adj.resetFlag = true; }
    
    /* If we're more than a full frame's worth
     * of ticks behind the ideal timestep,
     * there's no choice but to skip frame(s)
     * to catch up */
    bool frameSkipRequired() const {
        if (disabled)
            return false;
        
        return adj.idealDiff > tpf;
    }
    
private:
    void delayTicks(uint64_t ticks) {
#if defined(HAVE_NANOSLEEP)
        struct timespec req;
        uint64_t nsec = ticks / tickFreqNS;
        req.tv_sec = nsec / NS_PER_S;
        req.tv_nsec = nsec % NS_PER_S;
        errno = 0;
        
        while (nanosleep(&req, &req) == -1) {
            int err = errno;
            errno = 0;
            
            if (err == EINTR)
                continue;
            
            Debug() << "nanosleep failed. errno:" << err;
            SDL_Delay(ticks / tickFreqMS);
            break;
        }
#else
        SDL_Delay(ticks / tickFreqMS);
#endif
    }
};

struct GraphicsPrivate {
    /* Screen resolution, ie. the resolution at which
     * RGSS renders at (settable with Graphics.resize_screen).
     * Can only be changed from within RGSS */
    Vec2i scRes;
    
    /* Screen size, to which the rendered frames are scaled up.
     * This can be smaller than the window size when fixed aspect
     * ratio is enforced */
    Vec2i scSize;
    
    /* Actual physical size of the game window */
    Vec2i winSize;
    
    /* Offset in the game window at which the scaled game screen
     * is blitted inside the game window */
    Vec2i scOffset;
    
    // Scaling factor, used to display the screen properly
    // on Retina displays
    int scalingFactor;
    
    ScreenScene screen;
    RGSSThreadData *threadData;
    SDL_GLContext glCtx;
    
    int frameRate;
    int frameCount;
    int brightness;
    
    unsigned long long last_update;
    
    
    FPSLimiter fpsLimiter;
    
    // Can be set from Ruby. Takes priority over config setting.
    bool useFrameSkip;
    
    bool frozen;
    TEXFBO frozenScene;
    Quad screenQuad;
    
    std::vector<unsigned long long> avgFPSData;
    unsigned long long last_avg_update;
    SDL_mutex *avgFPSLock;
    
    SDL_mutex *glResourceLock;
    
    /* Global list of all live Disposables
     * (disposed on reset) */
    IntruList<Disposable> dispList;
    
    GraphicsPrivate(RGSSThreadData *rtData)
    : scRes(DEF_SCREEN_W, DEF_SCREEN_H), scSize(scRes),
    winSize(rtData->config.defScreenW, rtData->config.defScreenH),
    screen(scRes.x, scRes.y), threadData(rtData),
    glCtx(SDL_GL_GetCurrentContext()), frameRate(DEF_FRAMERATE),
    frameCount(0), brightness(255), fpsLimiter(frameRate),
    useFrameSkip(rtData->config.frameSkip), frozen(false),
    last_update(0), last_avg_update(0), scalingFactor(rtData->scale){
        avgFPSData = std::vector<unsigned long long>();
        avgFPSLock = SDL_CreateMutex();
        glResourceLock = SDL_CreateMutex();
        
        recalculateScreenSize(rtData);
        updateScreenResoRatio(rtData);
        
        TEXFBO::init(frozenScene);
        TEXFBO::allocEmpty(frozenScene, scRes.x, scRes.y);
        TEXFBO::linkFBO(frozenScene);
        
        FloatRect screenRect(0, 0, scRes.x, scRes.y);
        screenQuad.setTexPosRect(screenRect, screenRect);
        
        fpsLimiter.resetFrameAdjust();
    }
    
    ~GraphicsPrivate() {
        TEXFBO::fini(frozenScene);
        SDL_DestroyMutex(avgFPSLock);
        SDL_DestroyMutex(glResourceLock);
    }
    
    void updateScreenResoRatio(RGSSThreadData *rtData) {
        Vec2 &ratio = rtData->sizeResoRatio;
        ratio.x = (float)scRes.x / scSize.x;
        ratio.y = (float)scRes.y / scSize.y;
        
        rtData->screenOffset = scOffset;
    }
    
    /* Enforces fixed aspect ratio, if desired */
    void recalculateScreenSize(RGSSThreadData *rtData) {
        scSize = winSize;
        
        if (!rtData->config.fixedAspectRatio) {
            scOffset = Vec2i(0, 0);
            return;
        }
        
        float resRatio = (float)scRes.x / scRes.y;
        float winRatio = (float)winSize.x / winSize.y;
        
        if (resRatio > winRatio)
            scSize.y = scSize.x / resRatio;
        else if (resRatio < winRatio)
            scSize.x = scSize.y * resRatio;
        
        scOffset.x = (winSize.x - scSize.x) / 2.f;
        scOffset.y = (winSize.y - scSize.y) / 2.f;
    }
    
    void checkResize() {
        if (threadData->windowSizeMsg.poll(winSize)) {
            /* some GL drivers change the viewport on window resize */
            glState.viewport.refresh();
            recalculateScreenSize(threadData);
            updateScreenResoRatio(threadData);
            
            SDL_Rect screen = {scOffset.x, scOffset.y, scSize.x, scSize.y};
            threadData->ethread->notifyGameScreenChange(screen);
        }
    }
    
    void checkShutDownReset() {
        shState->checkShutdown();
        shState->checkReset();
    }
    
    void shutdown() {
        threadData->rqTermAck.set();
        shState->texPool().disable();
        
        scriptBinding->terminate();
    }
    
    void swapGLBuffer() {
        fpsLimiter.delay();
        SDL_GL_SwapWindow(threadData->window);
        
        ++frameCount;
        
        threadData->ethread->notifyFrame();
    }
    
    void compositeToBuffer(TEXFBO &buffer) {
        screen.composite();
        
        GLMeta::blitBegin(buffer);
        GLMeta::blitSource(screen.getPP().frontBuffer());
        GLMeta::blitRectangle(IntRect(0, 0, scRes.x, scRes.y), Vec2i());
        GLMeta::blitEnd();
    }
    
    void metaBlitBufferFlippedScaled() {
        GLMeta::blitRectangle(
                              IntRect(0, 0, scRes.x, scRes.y),
                              IntRect(scOffset.x * scalingFactor,
                                      (scSize.y + scOffset.y) * scalingFactor,
                                      scSize.x * scalingFactor,
                                      -scSize.y * scalingFactor),
                              threadData->config.smoothScaling);
    }
    
    void redrawScreen() {
        screen.composite();
        
        GLMeta::blitBeginScreen(winSize);
        GLMeta::blitSource(screen.getPP().frontBuffer());
        
        FBO::clear();
        metaBlitBufferFlippedScaled();
        
        GLMeta::blitEnd();
        
        swapGLBuffer();
        
        SDL_LockMutex(avgFPSLock);
        if (avgFPSData.size() > 40)
            avgFPSData.erase(avgFPSData.begin());
        
        unsigned long long time = shState->runTime();
        avgFPSData.push_back(time - last_avg_update);
        last_avg_update = time;
        SDL_UnlockMutex(avgFPSLock);
    }
    
    void checkSyncLock() {
        if (!threadData->syncPoint.mainSyncLocked())
            return;
        
        /* Releasing the GL context before sleeping and making it
         * current again on wakeup seems to avoid the context loss
         * when the app moves into the background on Android */
        SDL_GL_MakeCurrent(threadData->window, 0);
        threadData->syncPoint.waitMainSync();
        SDL_GL_MakeCurrent(threadData->window, glCtx);
        
        fpsLimiter.resetFrameAdjust();
    }
    
    double averageFPS() {
        double ret = 0;
        SDL_LockMutex(avgFPSLock);
        for (unsigned long long times : avgFPSData)
            ret += times;
        
        ret = 1 / (ret / avgFPSData.size() / 1000000);
        SDL_UnlockMutex(avgFPSLock);
        return ret;
    }
    
    void setLock() {
        SDL_LockMutex(glResourceLock);
        SDL_GL_MakeCurrent(threadData->window, threadData->glContext);
    }
    
    void releaseLock() {
        SDL_UnlockMutex(glResourceLock);
    }
};

Graphics::Graphics(RGSSThreadData *data) {
    p = new GraphicsPrivate(data);
    if (data->config.syncToRefreshrate) {
        p->frameRate = data->refreshRate;
        p->fpsLimiter.disabled = true;
    } else if (data->config.fixedFramerate > 0) {
        p->fpsLimiter.setDesiredFPS(data->config.fixedFramerate);
    } else if (data->config.fixedFramerate < 0) {
        p->fpsLimiter.disabled = true;
    }
}

Graphics::~Graphics() { delete p; }

unsigned long long Graphics::getDelta() {
    return shState->runTime() - p->last_update;
}

unsigned long long Graphics::lastUpdate() {
    return p->last_update;
}

void Graphics::update(bool checkForShutdown) {
    p->threadData->rqWindowAdjust.wait();
    p->last_update = shState->runTime();
    
    if (checkForShutdown)
        p->checkShutDownReset();
    
    p->checkSyncLock();

    
#ifdef MKXPZ_STEAM
    if (STEAMSHIM_alive())
        STEAMSHIM_pump();
#endif
    
    if (p->frozen)
        return;
    
    if (p->fpsLimiter.frameSkipRequired()) {
        if (p->useFrameSkip) {
            /* Skip frame */
            p->fpsLimiter.delay();
            ++p->frameCount;
            p->threadData->ethread->notifyFrame();
            
            return;
        } else {
            /* Just reset frame adjust counter */
            p->fpsLimiter.resetFrameAdjust();
        }
    }
    
    p->checkResize();
    p->redrawScreen();
}

void Graphics::freeze() {
    p->frozen = true;
    
    p->checkShutDownReset();
    p->checkResize();
    
    /* Capture scene into frozen buffer */
    p->compositeToBuffer(p->frozenScene);
}

void Graphics::transition(int duration, const char *filename, int vague) {
    p->checkSyncLock();
    
    if (!p->frozen)
        return;
    
    vague = clamp(vague, 1, 256);
    Bitmap *transMap = *filename ? new Bitmap(filename) : 0;
    
    setBrightness(255);
    
    /* Capture new scene */
    p->screen.composite();
    
    /* The PP frontbuffer will hold the current scene after the
     * composition step. Since the backbuffer is unused during
     * the transition, we can reuse it as the target buffer for
     * the final rendered image. */
    TEXFBO &currentScene = p->screen.getPP().frontBuffer();
    TEXFBO &transBuffer = p->screen.getPP().backBuffer();
    
    /* If no transition bitmap is provided,
     * we can use a simplified shader */
    TransShader &transShader = shState->shaders().trans;
    SimpleTransShader &simpleShader = shState->shaders().simpleTrans;
    
    if (transMap) {
        TransShader &shader = transShader;
        shader.bind();
        shader.applyViewportProj();
        shader.setFrozenScene(p->frozenScene.tex);
        shader.setCurrentScene(currentScene.tex);
        shader.setTransMap(transMap->getGLTypes().tex);
        shader.setVague(vague / 256.0f);
        shader.setTexSize(p->scRes);
    } else {
        SimpleTransShader &shader = simpleShader;
        shader.bind();
        shader.applyViewportProj();
        shader.setFrozenScene(p->frozenScene.tex);
        shader.setCurrentScene(currentScene.tex);
        shader.setTexSize(p->scRes);
    }
    
    glState.blend.pushSet(false);
    
    for (int i = 0; i < duration; ++i) {
        /* We need to clean up transMap properly before
         * a possible longjmp, so we manually test for
         * shutdown/reset here */
        if (p->threadData->rqTerm) {
            glState.blend.pop();
            delete transMap;
            p->shutdown();
            return;
        }
        
        if (p->threadData->rqReset) {
            glState.blend.pop();
            delete transMap;
            scriptBinding->reset();
            return;
        }
        
        p->checkSyncLock();
        
        const float prog = i * (1.0f / duration);
        
        if (transMap) {
            transShader.bind();
            transShader.setProg(prog);
        } else {
            simpleShader.bind();
            simpleShader.setProg(prog);
        }
        
        /* Draw the composed frame to a buffer first
         * (we need this because we're skipping PingPong) */
        FBO::bind(transBuffer.fbo);
        FBO::clear();
        p->screenQuad.draw();
        
        p->checkResize();
        
        /* Then blit it flipped and scaled to the screen */
        FBO::unbind();
        FBO::clear();
        
        GLMeta::blitBeginScreen(Vec2i(p->winSize));
        GLMeta::blitSource(transBuffer);
        p->metaBlitBufferFlippedScaled();
        GLMeta::blitEnd();
        
        p->swapGLBuffer();
    }
    
    glState.blend.pop();
    
    delete transMap;
    
    p->frozen = false;
}

void Graphics::frameReset() {p->fpsLimiter.resetFrameAdjust();}

static void guardDisposed() {}

DEF_ATTR_RD_SIMPLE(Graphics, FrameRate, int, p->frameRate)

DEF_ATTR_SIMPLE(Graphics, FrameCount, int, p->frameCount)

void Graphics::setFrameRate(int value) {
    p->frameRate = clamp(value, 10, 120);
    
    if (p->threadData->config.syncToRefreshrate)
        return;
    
    if (p->threadData->config.fixedFramerate > 0)
        return;
    
    p->fpsLimiter.setDesiredFPS(p->frameRate);
    shState->input().recalcRepeat((unsigned int)p->frameRate);
}

double Graphics::averageFrameRate() {
    return p->averageFPS();
}

void Graphics::wait(int duration) {
    for (int i = 0; i < duration; ++i) {
        p->checkShutDownReset();
        p->redrawScreen();
    }
}

void Graphics::fadeout(int duration) {
    FBO::unbind();
    
    float curr = p->brightness;
    float diff = 255.0f - curr;
    
    for (int i = duration - 1; i > -1; --i) {
        setBrightness(diff + (curr / duration) * i);
        
        if (p->frozen) {
            GLMeta::blitBeginScreen(p->scSize);
            GLMeta::blitSource(p->frozenScene);
            
            FBO::clear();
            p->metaBlitBufferFlippedScaled();
            
            GLMeta::blitEnd();
            
            p->swapGLBuffer();
        } else {
            update();
        }
    }
}

void Graphics::fadein(int duration) {
    FBO::unbind();
    
    float curr = p->brightness;
    float diff = 255.0f - curr;
    
    for (int i = 1; i <= duration; ++i) {
        setBrightness(curr + (diff / duration) * i);
        
        if (p->frozen) {
            GLMeta::blitBeginScreen(p->scSize);
            GLMeta::blitSource(p->frozenScene);
            
            FBO::clear();
            p->metaBlitBufferFlippedScaled();
            
            GLMeta::blitEnd();
            
            p->swapGLBuffer();
        } else {
            update();
        }
    }
}

Bitmap *Graphics::snapToBitmap() {
    Bitmap *bitmap = new Bitmap(width(), height());

    p->compositeToBuffer(bitmap->getGLTypes());
    
    /* Taint entire bitmap */
    bitmap->taintArea(IntRect(0, 0, width(), height()));
    return bitmap;
}

int Graphics::width() const { return p->scRes.x; }

int Graphics::height() const { return p->scRes.y; }

void Graphics::resizeScreen(int width, int height) {
    // width = clamp(width, 1, 640);
    // height = clamp(height, 1, 480);
    
    p->threadData->rqWindowAdjust.wait();
    Vec2i size(width, height);
    
    if (p->scRes == size)
        return;
    
    p->scRes = size;
    
    p->screen.setResolution(width, height);
    
    TEXFBO::allocEmpty(p->frozenScene, width, height);
    
    FloatRect screenRect(0, 0, width, height);
    p->screenQuad.setTexPosRect(screenRect, screenRect);
    
    glState.scissorBox.set(IntRect(0, 0, p->scRes.x, p->scRes.y));
    
    
    p->threadData->rqWindowAdjust.set();
    shState->eThread().requestWindowResize(width, height);
    update();
}

bool Graphics::updateMovieInput(Movie *movie) {
    return  p->threadData->rqTerm || p->threadData->rqReset;
}

void Graphics::playMovie(const char *filename, int volume, bool skippable) {
    Movie *movie = new Movie(volume, skippable);
    MovieOpenHandler handler(movie->srcOps);
    shState->fileSystem().openRead(handler, filename);

    if (movie->preparePlayback()) {
        int limiterDisabled = p->fpsLimiter.disabled;
        p->fpsLimiter.disabled = false;
        int oldFPS = getFrameRate();
        setFrameRate(movie->video->fps);
        bool oldframeskip = p->useFrameSkip;
        p->useFrameSkip = false;
        update();
        
        Sprite movieSprite;
        
        // Currently this stretches to fit the screen. VX Ace behavior is to center it and let the edges run off
        movieSprite.setBitmap(movie->videoBitmap);
        double ratio = std::min((double)width() / movie->video->width, (double)height() / movie->video->height);
        movieSprite.setZoomX(ratio);
        movieSprite.setZoomY(ratio);
        movieSprite.setX((width() / 2) - (movie->video->width * ratio / 2));
        movieSprite.setY((height() / 2) - (movie->video->height * ratio / 2));
        
        Sprite letterboxSprite;
        Bitmap letterbox(width(), height());
        letterbox.fillRect(0, 0, width(), height(), Vec4(0,0,0,255));
        letterboxSprite.setBitmap(&letterbox);
        
        letterboxSprite.setZ(-1000);
        movieSprite.setZ(-998);

        movie->play();
        
        p->fpsLimiter.disabled = limiterDisabled;
        setFrameRate(oldFPS);
        p->useFrameSkip = oldframeskip;
    }

    delete movie;
}

void Graphics::screenshot(const char *filename) {
    p->threadData->rqWindowAdjust.wait();
    Bitmap *ss = snapToBitmap();
    ss->saveToFile(filename);
    ss->dispose();
    delete ss;
}

DEF_ATTR_RD_SIMPLE(Graphics, Brightness, int, p->brightness)

void Graphics::setBrightness(int value) {
    value = clamp(value, 0, 255);
    
    if (p->brightness == value)
        return;
    
    p->brightness = value;
    p->screen.setBrightness(value / 255.0);
}

void Graphics::reset() {
    /* Dispose all live Disposables */
    IntruListLink<Disposable> *iter;
    
    for (iter = p->dispList.begin(); iter != p->dispList.end();
         iter = iter->next) {
        iter->data->dispose();
    }
    
    p->dispList.clear();
    
    /* Reset attributes (frame count not included) */
    p->fpsLimiter.resetFrameAdjust();
    p->frozen = false;
    p->screen.getPP().clearBuffers();
    
    setFrameRate(DEF_FRAMERATE);
    setBrightness(255);
}

void Graphics::center() {
    if (getFullscreen())
        return;
    
    p->threadData->rqWindowAdjust.reset();
    p->threadData->ethread->requestWindowCenter();
}

bool Graphics::getFullscreen() const {
    return p->threadData->ethread->getFullscreen();
}

void Graphics::setFullscreen(bool value) {
    p->threadData->ethread->requestFullscreenMode(value);
}

bool Graphics::getShowCursor() const {
    return p->threadData->ethread->getShowCursor();
}

void Graphics::setShowCursor(bool value) {
    p->threadData->ethread->requestShowCursor(value);
}

double Graphics::getScale() const { return (double)p->scSize.y / p->scRes.y; }

void Graphics::setScale(double factor) {
    p->threadData->rqWindowAdjust.wait();
    factor = clamp(factor, 0.5, 2.0);
    
    if (factor == getScale())
        return;
    
    int widthpx = p->scRes.x * factor;
    int heightpx = p->scRes.y * factor;
    
    p->threadData->rqWindowAdjust.set();
    shState->eThread().requestWindowResize(widthpx, heightpx);
    update();
}

bool Graphics::getFrameskip() const { return p->useFrameSkip; }

void Graphics::setFrameskip(bool value) { p->useFrameSkip = value; }

Scene *Graphics::getScreen() const { return &p->screen; }

void Graphics::repaintWait(const AtomicFlag &exitCond, bool checkReset) {
    if (exitCond)
        return;
    
    /* Repaint the screen with the last good frame we drew */
    TEXFBO &lastFrame = p->screen.getPP().frontBuffer();
    GLMeta::blitBeginScreen(p->winSize);
    GLMeta::blitSource(lastFrame);
    
    while (!exitCond) {
        shState->checkShutdown();
        
        if (checkReset)
            shState->checkReset();
        
        FBO::clear();
        p->metaBlitBufferFlippedScaled();
        SDL_GL_SwapWindow(p->threadData->window);
        p->fpsLimiter.delay();
        
        p->threadData->ethread->notifyFrame();
    }
    
    GLMeta::blitEnd();
}

void Graphics::lock() {
    p->setLock();
}

void Graphics::unlock() {
    p->releaseLock();
}

void Graphics::addDisposable(Disposable *d) { p->dispList.append(d->link); }

void Graphics::remDisposable(Disposable *d) { p->dispList.remove(d->link); }

#undef GRAPHICS_THREAD_LOCK
