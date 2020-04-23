# ======================================================================
# TILEMAP VERTICAL WRAPPER
#
# This is a little fix for Pokemon Essentials' custom tilemap code
# that works around MKXP's GPU texture size limit that would normally
# stop you from playing a lot of games.
#
# The concept is simple enough: If your tileset is too big, a new
# bitmap will be constructed with all the excess pixels sent to the
# image's right side. This basically means that you now have a limit
# far higher than you should ever actually need.
#
# Because of the extra math the game will have to do to find the right
# pixels, this will probably cause a performance hit while on these
# maps which would normally be megasurfaces.
#
# It can probably be improved by changing CustomTilemap.getRegularTile
# to just find the correct pixels on its own, without having to
# translate the coordinates afterwards, but this will satisfy me just
# for the moment.
#
# Really, it'd be far better to cut up the image on the C++ end and
# use a custom shader to get the image right or something like that,
# but that's work for another day.
#
# For now, I'm just happy I can finally test whatever game I like.
#                             ~Zoro
#=======================================================================

module VWrap

  MAX_TEX_SIZE = Bitmap.max_size
  TILESET_WIDTH = 0x100

  def self.clamp(val, min, max)
    val = max if val > max
    val = min if val < min
    return val
  end

  def self.makeVWrappedTileset(originalbmp)
    width = originalbmp.width
    height = originalbmp.height
    if width == TILESET_WIDTH && originalbmp.mega?
      columns = (height / MAX_TEX_SIZE.to_f).ceil
      
      return nil if columns * TILESET_WIDTH > MAX_TEX_SIZE
      bmp = Bitmap.new(TILESET_WIDTH*columns, MAX_TEX_SIZE)
      remainder = height % MAX_TEX_SIZE
      
      columns.times{|col|
        srcrect = Rect.new(0, col * MAX_TEX_SIZE, width, (col + 1 == columns) ? remainder : MAX_TEX_SIZE)
        bmp.blt(col*TILESET_WIDTH, 0, originalbmp, srcrect)
      }
      return bmp
    end

    return originalbmp
  end

  def self.blitVWrappedPixels(destX, destY, dest, src, srcrect)
    merge = (srcrect.y % MAX_TEX_SIZE) > ((srcrect.y + srcrect.height) % MAX_TEX_SIZE)

    srcrect.x = clamp(srcrect.x, 0,TILESET_WIDTH)
    srcrect.width = clamp(srcrect.width, 0, TILESET_WIDTH - srcrect.x)
    col = (srcrect.y / MAX_TEX_SIZE.to_f).floor
    srcX = col * TILESET_WIDTH + srcrect.x
    srcY = srcrect.y % MAX_TEX_SIZE

    if !merge
      dest.blt(destX, destY, src, Rect.new(srcX, srcY, srcrect.width, srcrect.height))
    else
      #FIXME won't work on heights longer than two columns, but nobody should need
      # more than 32k pixels high at once anyway
      side = {:a => MAX_TEX_SIZE - srcY, :b => srcrect.height - (MAX_TEX_SIZE - srcY)}
      dest.blt(destX, destY, src, Rect.new(srcX, srcY, srcrect.width, side[:a]))
      dest.blt(destX, destY + side[:a], src, Rect.new(srcX + TILESET_WIDTH, 0, srcrect.width, side[:b]))
    end
  end
end

# -------------------------
if $MKXP == true
  class CustomTilemap
    def tileset=(value)
      if value.mega?
        @tileset = VWrap::makeVWrappedTileset(value)
      else
        @tileset = value
      end
      @tilesetchanged = true
    end

    alias old_getRegularTile getRegularTile
    def getRegularTile(sprite, id)
      return old_getRegularTile(sprite, id) if @tileset.width <= VWrap::TILESET_WIDTH

      bitmap = @regularTileInfo[id]
      if !bitmap
        bitmap = Bitmap.new(@tileWidth, @tileHeight)
        rect=Rect.new(((id - 384)&7)*@tileSrcWidth,((id - 384)>>3)*@tileSrcHeight,
           @tileSrcWidth,@tileSrcHeight)
           VWrap::blitVWrappedPixels(0,0, bitmap, @tileset, rect)
        @regularTileInfo[id]=bitmap
      end
      sprite.bitmap = bitmap if sprite.bitmap != bitmap
    end
  end
end

# =================================================================
# Any extra overrides to fix a bunch of break-y things.
# This should allow someone to load up games on Windows just fine.
# Maybe not perfect compatibility, but better.
# =================================================================

module Graphics
  def self.snap_to_bitmap
    return Graphics.mkxp_snap_to_bitmap
  end
end

def pbScreenCapture
  capturefile = nil
  500.times{|i|
    filename = RTP.getSaveFileName(sprintf("capture%03d.bmp",i))
    if !safeExists?(filename)
      capturefile = filename
      break
    end
  }
  begin
    Graphics.screenshot(capturefile)
    pbSEPlay("expfull") if FileTest.audio_exist?("Audio/SE/expfull")
  rescue
    nil
  end
end

alias old_pbDrawTextPositions pbDrawTextPositions
def pbDrawTextPositions(bitmap,textpos)
  old_pbDrawTextPositions(bitmap,textpos.map{|n|n+4})
end
