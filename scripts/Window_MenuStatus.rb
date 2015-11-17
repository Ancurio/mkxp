#==============================================================================
# ** Window_MenuStatus
#------------------------------------------------------------------------------
#  This window displays party member status on the menu screen.
#==============================================================================

class Window_MenuStatus < Window_Selectable
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    super(0, 0, 480, 480)
    self.contents = Bitmap.new(width - 32, height - 32)
    refresh
    self.active = false
    self.index = -1
  end
  #--------------------------------------------------------------------------
  # * Refresh
  #--------------------------------------------------------------------------
  def refresh
    self.contents.clear
    @item_max = $game_party.actors.size
    for i in 0...$game_party.actors.size
      x = 64
      y = i * 116
      actor = $game_party.actors[i]
      draw_actor_graphic(actor, x - 40, y + 80)
      draw_actor_name(actor, x, y)
      draw_actor_class(actor, x + 144, y)
      draw_actor_level(actor, x, y + 32)
      draw_actor_state(actor, x + 90, y + 32)
      draw_actor_exp(actor, x, y + 64)
      draw_actor_hp(actor, x + 236, y + 32)
      draw_actor_sp(actor, x + 236, y + 64)
    end
  end
  #--------------------------------------------------------------------------
  # * Cursor Rectangle Update
  #--------------------------------------------------------------------------
  def update_cursor_rect
    if @index < 0
      self.cursor_rect.empty
    else
      self.cursor_rect.set(0, @index * 116, self.width - 32, 96)
    end
  end
end
