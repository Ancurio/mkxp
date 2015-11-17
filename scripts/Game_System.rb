#==============================================================================
# ** Game_System
#------------------------------------------------------------------------------
#  This class handles data surrounding the system. Backround music, etc.
#  is managed here as well. Refer to "$game_system" for the instance of
#  this class.
#==============================================================================

class Game_System
  #--------------------------------------------------------------------------
  # * Public Instance Variables
  #--------------------------------------------------------------------------
  attr_reader   :map_interpreter          # map event interpreter
  attr_reader   :battle_interpreter       # battle event interpreter
  attr_accessor :timer                    # timer
  attr_accessor :timer_working            # timer working flag
  attr_accessor :save_disabled            # save forbidden
  attr_accessor :menu_disabled            # menu forbidden
  attr_accessor :encounter_disabled       # encounter forbidden
  attr_accessor :message_position         # text option: positioning
  attr_accessor :message_frame            # text option: window frame
  attr_accessor :save_count               # save count
  attr_accessor :magic_number             # magic number
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    @map_interpreter = Interpreter.new(0, true)
    @battle_interpreter = Interpreter.new(0, false)
    @timer = 0
    @timer_working = false
    @save_disabled = false
    @menu_disabled = false
    @encounter_disabled = false
    @message_position = 2
    @message_frame = 0
    @save_count = 0
    @magic_number = 0
  end
  #--------------------------------------------------------------------------
  # * Play Background Music
  #     bgm : background music to be played
  #--------------------------------------------------------------------------
  def bgm_play(bgm)
    @playing_bgm = bgm
    if bgm != nil and bgm.name != ""
      Audio.bgm_play("Audio/BGM/" + bgm.name, bgm.volume, bgm.pitch)
    else
      Audio.bgm_stop
    end
    Graphics.frame_reset
  end
  #--------------------------------------------------------------------------
  # * Stop Background Music
  #--------------------------------------------------------------------------
  def bgm_stop
    Audio.bgm_stop
  end
  #--------------------------------------------------------------------------
  # * Fade Out Background Music
  #     time : fade-out time (in seconds)
  #--------------------------------------------------------------------------
  def bgm_fade(time)
    @playing_bgm = nil
    Audio.bgm_fade(time * 1000)
  end
  #--------------------------------------------------------------------------
  # * Background Music Memory
  #--------------------------------------------------------------------------
  def bgm_memorize
    @memorized_bgm = @playing_bgm
  end
  #--------------------------------------------------------------------------
  # * Restore Background Music
  #--------------------------------------------------------------------------
  def bgm_restore
    bgm_play(@memorized_bgm)
  end
  #--------------------------------------------------------------------------
  # * Play Background Sound
  #     bgs : background sound to be played
  #--------------------------------------------------------------------------
  def bgs_play(bgs)
    @playing_bgs = bgs
    if bgs != nil and bgs.name != ""
      Audio.bgs_play("Audio/BGS/" + bgs.name, bgs.volume, bgs.pitch)
    else
      Audio.bgs_stop
    end
    Graphics.frame_reset
  end
  #--------------------------------------------------------------------------
  # * Fade Out Background Sound
  #     time : fade-out time (in seconds)
  #--------------------------------------------------------------------------
  def bgs_fade(time)
    @playing_bgs = nil
    Audio.bgs_fade(time * 1000)
  end
  #--------------------------------------------------------------------------
  # * Background Sound Memory
  #--------------------------------------------------------------------------
  def bgs_memorize
    @memorized_bgs = @playing_bgs
  end
  #--------------------------------------------------------------------------
  # * Restore Background Sound
  #--------------------------------------------------------------------------
  def bgs_restore
    bgs_play(@memorized_bgs)
  end
  #--------------------------------------------------------------------------
  # * Play Music Effect
  #     me : music effect to be played
  #--------------------------------------------------------------------------
  def me_play(me)
    if me != nil and me.name != ""
      Audio.me_play("Audio/ME/" + me.name, me.volume, me.pitch)
    else
      Audio.me_stop
    end
    Graphics.frame_reset
  end
  #--------------------------------------------------------------------------
  # * Play Sound Effect
  #     se : sound effect to be played
  #--------------------------------------------------------------------------
  def se_play(se)
    if se != nil and se.name != ""
      Audio.se_play("Audio/SE/" + se.name, se.volume, se.pitch)
    end
  end
  #--------------------------------------------------------------------------
  # * Stop Sound Effect
  #--------------------------------------------------------------------------
  def se_stop
    Audio.se_stop
  end
  #--------------------------------------------------------------------------
  # * Get Playing Background Music
  #--------------------------------------------------------------------------
  def playing_bgm
    return @playing_bgm
  end
  #--------------------------------------------------------------------------
  # * Get Playing Background Sound
  #--------------------------------------------------------------------------
  def playing_bgs
    return @playing_bgs
  end
  #--------------------------------------------------------------------------
  # * Get Windowskin File Name
  #--------------------------------------------------------------------------
  def windowskin_name
    if @windowskin_name == nil
      return $data_system.windowskin_name
    else
      return @windowskin_name
    end
  end
  #--------------------------------------------------------------------------
  # * Set Windowskin File Name
  #     windowskin_name : new windowskin file name
  #--------------------------------------------------------------------------
  def windowskin_name=(windowskin_name)
    @windowskin_name = windowskin_name
  end
  #--------------------------------------------------------------------------
  # * Get Battle Background Music
  #--------------------------------------------------------------------------
  def battle_bgm
    if @battle_bgm == nil
      return $data_system.battle_bgm
    else
      return @battle_bgm
    end
  end
  #--------------------------------------------------------------------------
  # * Set Battle Background Music
  #     battle_bgm : new battle background music
  #--------------------------------------------------------------------------
  def battle_bgm=(battle_bgm)
    @battle_bgm = battle_bgm
  end
  #--------------------------------------------------------------------------
  # * Get Background Music for Battle Ending
  #--------------------------------------------------------------------------
  def battle_end_me
    if @battle_end_me == nil
      return $data_system.battle_end_me
    else
      return @battle_end_me
    end
  end
  #--------------------------------------------------------------------------
  # * Set Background Music for Battle Ending
  #     battle_end_me : new battle ending background music
  #--------------------------------------------------------------------------
  def battle_end_me=(battle_end_me)
    @battle_end_me = battle_end_me
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    # reduce timer by 1
    if @timer_working and @timer > 0
      @timer -= 1
    end
  end
end
