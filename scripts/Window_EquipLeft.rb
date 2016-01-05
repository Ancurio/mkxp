#==============================================================================
# ** Window_EquipLeft
#------------------------------------------------------------------------------
#  This window displays actor parameter changes on the equipment screen.
#==============================================================================

class Window_EquipLeft < Window_Base
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     actor : actor
  #--------------------------------------------------------------------------
  def initialize(actor)
    super(0, 64, 272, 192)
    self.contents = Bitmap.new(width - 32, height - 32)
    @actor = actor
    refresh
  end
  #--------------------------------------------------------------------------
  # * Refresh
  #--------------------------------------------------------------------------
  def refresh
    self.contents.clear
    draw_actor_name(@actor, 4, 0)
    draw_actor_level(@actor, 4, 32)
    draw_actor_parameter(@actor, 4, 64, 0)
    draw_actor_parameter(@actor, 4, 96, 1)
    draw_actor_parameter(@actor, 4, 128, 2)
    if @new_atk != nil
      self.contents.font.color = system_color
      self.contents.draw_text(160, 64, 40, 32, "->", 1)
      self.contents.font.color = normal_color
      self.contents.draw_text(200, 64, 36, 32, @new_atk.to_s, 2)
    end
    if @new_pdef != nil
      self.contents.font.color = system_color
      self.contents.draw_text(160, 96, 40, 32, "->", 1)
      self.contents.font.color = normal_color
      self.contents.draw_text(200, 96, 36, 32, @new_pdef.to_s, 2)
    end
    if @new_mdef != nil
      self.contents.font.color = system_color
      self.contents.draw_text(160, 128, 40, 32, "->", 1)
      self.contents.font.color = normal_color
      self.contents.draw_text(200, 128, 36, 32, @new_mdef.to_s, 2)
    end
  end
  #--------------------------------------------------------------------------
  # * Set parameters after changing equipment
  #     new_atk  : attack power after changing equipment
  #     new_pdef : physical defense after changing equipment
  #     new_mdef : magic defense after changing equipment
  #--------------------------------------------------------------------------
  def set_new_parameters(new_atk, new_pdef, new_mdef)
    if @new_atk != new_atk or @new_pdef != new_pdef or @new_mdef != new_mdef
      @new_atk = new_atk
      @new_pdef = new_pdef
      @new_mdef = new_mdef
      refresh
    end
  end
end
