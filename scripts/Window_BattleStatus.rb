#==============================================================================
# ** Window_BattleStatus
#------------------------------------------------------------------------------
#  This window displays the status of all party members on the battle screen.
#==============================================================================

class Window_BattleStatus < Window_Base
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    super(0, 320, 640, 160)
    self.contents = Bitmap.new(width - 32, height - 32)
    @level_up_flags = [false, false, false, false]
    refresh
  end
  #--------------------------------------------------------------------------
  # * Dispose
  #--------------------------------------------------------------------------
  def dispose
    super
  end
  #--------------------------------------------------------------------------
  # * Set Level Up Flag
  #     actor_index : actor index
  #--------------------------------------------------------------------------
  def level_up(actor_index)
    @level_up_flags[actor_index] = true
  end
  #--------------------------------------------------------------------------
  # * Refresh
  #--------------------------------------------------------------------------
  def refresh
    self.contents.clear
    @item_max = $game_party.actors.size
    for i in 0...$game_party.actors.size
      actor = $game_party.actors[i]
      actor_x = i * 160 + 4
      draw_actor_name(actor, actor_x, 0)
      draw_actor_hp(actor, actor_x, 32, 120)
      draw_actor_sp(actor, actor_x, 64, 120)
      if @level_up_flags[i]
        self.contents.font.color = normal_color
        self.contents.draw_text(actor_x, 96, 120, 32, "LEVEL UP!")
      else
        draw_actor_state(actor, actor_x, 96)
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    super
    # Slightly lower opacity level during main phase
    if $game_temp.battle_main_phase
      self.contents_opacity -= 4 if self.contents_opacity > 191
    else
      self.contents_opacity += 4 if self.contents_opacity < 255
    end
  end
end
