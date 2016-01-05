#==============================================================================
# ** Window_Status
#------------------------------------------------------------------------------
#  This window displays full status specs on the status screen.
#==============================================================================

class Window_Status < Window_Base
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     actor : actor
  #--------------------------------------------------------------------------
  def initialize(actor)
    super(0, 0, 640, 480)
    self.contents = Bitmap.new(width - 32, height - 32)
    @actor = actor
    refresh
  end
  #--------------------------------------------------------------------------
  # * Refresh
  #--------------------------------------------------------------------------
  def refresh
    self.contents.clear
    draw_actor_graphic(@actor, 40, 112)
    draw_actor_name(@actor, 4, 0)
    draw_actor_class(@actor, 4 + 144, 0)
    draw_actor_level(@actor, 96, 32)
    draw_actor_state(@actor, 96, 64)
    draw_actor_hp(@actor, 96, 112, 172)
    draw_actor_sp(@actor, 96, 144, 172)
    draw_actor_parameter(@actor, 96, 192, 0)
    draw_actor_parameter(@actor, 96, 224, 1)
    draw_actor_parameter(@actor, 96, 256, 2)
    draw_actor_parameter(@actor, 96, 304, 3)
    draw_actor_parameter(@actor, 96, 336, 4)
    draw_actor_parameter(@actor, 96, 368, 5)
    draw_actor_parameter(@actor, 96, 400, 6)
    self.contents.font.color = system_color
    self.contents.draw_text(320, 48, 80, 32, "EXP")
    self.contents.draw_text(320, 80, 80, 32, "NEXT")
    self.contents.font.color = normal_color
    self.contents.draw_text(320 + 80, 48, 84, 32, @actor.exp_s, 2)
    self.contents.draw_text(320 + 80, 80, 84, 32, @actor.next_rest_exp_s, 2)
    self.contents.font.color = system_color
    self.contents.draw_text(320, 160, 96, 32, "equipment")
    draw_item_name($data_weapons[@actor.weapon_id], 320 + 16, 208)
    draw_item_name($data_armors[@actor.armor1_id], 320 + 16, 256)
    draw_item_name($data_armors[@actor.armor2_id], 320 + 16, 304)
    draw_item_name($data_armors[@actor.armor3_id], 320 + 16, 352)
    draw_item_name($data_armors[@actor.armor4_id], 320 + 16, 400)
  end
  def dummy
    self.contents.font.color = system_color
    self.contents.draw_text(320, 112, 96, 32, $data_system.words.weapon)
    self.contents.draw_text(320, 176, 96, 32, $data_system.words.armor1)
    self.contents.draw_text(320, 240, 96, 32, $data_system.words.armor2)
    self.contents.draw_text(320, 304, 96, 32, $data_system.words.armor3)
    self.contents.draw_text(320, 368, 96, 32, $data_system.words.armor4)
    draw_item_name($data_weapons[@actor.weapon_id], 320 + 24, 144)
    draw_item_name($data_armors[@actor.armor1_id], 320 + 24, 208)
    draw_item_name($data_armors[@actor.armor2_id], 320 + 24, 272)
    draw_item_name($data_armors[@actor.armor3_id], 320 + 24, 336)
    draw_item_name($data_armors[@actor.armor4_id], 320 + 24, 400)
  end
end
