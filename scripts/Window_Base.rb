#==============================================================================
# ** Window_Base
#------------------------------------------------------------------------------
#  This class is for all in-game windows.
#==============================================================================

class Window_Base < Window
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     x      : window x-coordinate
  #     y      : window y-coordinate
  #     width  : window width
  #     height : window height
  #--------------------------------------------------------------------------
  def initialize(x, y, width, height)
    super()
    @windowskin_name = $game_system.windowskin_name
    self.windowskin = RPG::Cache.windowskin(@windowskin_name)
    self.x = x
    self.y = y
    self.width = width
    self.height = height
    self.z = 100
  end
  #--------------------------------------------------------------------------
  # * Dispose
  #--------------------------------------------------------------------------
  def dispose
    # Dispose if window contents bit map is set
    if self.contents != nil
      self.contents.dispose
    end
    super
  end
  #--------------------------------------------------------------------------
  # * Get Text Color
  #     n : text color number (0-7)
  #--------------------------------------------------------------------------
  def self.text_color(n)
    case n
    when 0
      return Color.new(255, 255, 255, 255)
    when 1
      return Color.new(255, 128, 128, 255)
    when 2
      return Color.new(128, 255, 128, 255)
    when 3
      return Color.new(255, 255, 128, 255)
    when 4
      return Color.new(128, 128, 255, 255)
    when 5
      return Color.new(255, 128, 255, 255)
    when 6
      return Color.new(128, 255, 255, 255)
    when 7
      return Color.new(128, 128, 128, 255)
    else
      normal_color
    end
  end
  #--------------------------------------------------------------------------
  # * Get Normal Text Color
  #--------------------------------------------------------------------------
  def normal_color
    return Color.new(255, 255, 255, 255)
  end
  #--------------------------------------------------------------------------
  # * Get Active Item Text Color
  #--------------------------------------------------------------------------
  def active_item_color
    return Color.new(222, 134, 0, 255)
  end
  #--------------------------------------------------------------------------
  # * Get Disabled Text Color
  #--------------------------------------------------------------------------
  def disabled_color
    return Color.new(255, 255, 255, 128)
  end
  #--------------------------------------------------------------------------
  # * Get System Text Color
  #--------------------------------------------------------------------------
  def system_color
    return Color.new(192, 224, 255, 255)
  end
  #--------------------------------------------------------------------------
  # * Get Crisis Text Color
  #--------------------------------------------------------------------------
  def crisis_color
    return Color.new(255, 255, 64, 255)
  end
  #--------------------------------------------------------------------------
  # * Get Knockout Text Color
  #--------------------------------------------------------------------------
  def knockout_color
    return Color.new(255, 64, 0)
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    super
    # Reset if windowskin was changed
    if $game_system.windowskin_name != @windowskin_name
      @windowskin_name = $game_system.windowskin_name
      self.windowskin = RPG::Cache.windowskin(@windowskin_name)
    end
  end
  #--------------------------------------------------------------------------
  # * Draw Graphic
  #     actor : actor
  #     x     : draw spot x-coordinate
  #     y     : draw spot y-coordinate
  #--------------------------------------------------------------------------
  def draw_actor_graphic(actor, x, y)
    bitmap = RPG::Cache.character(actor.character_name, actor.character_hue)
    cw = bitmap.width / 4
    ch = bitmap.height / 4
    src_rect = Rect.new(0, 0, cw, ch)
    self.contents.blt(x - cw / 2, y - ch, bitmap, src_rect)
  end
  #--------------------------------------------------------------------------
  # * Draw Name
  #     actor : actor
  #     x     : draw spot x-coordinate
  #     y     : draw spot y-coordinate
  #--------------------------------------------------------------------------
  def draw_actor_name(actor, x, y)
    self.contents.font.color = normal_color
    self.contents.draw_text(x, y, 120, 32, actor.name)
  end
  #--------------------------------------------------------------------------
  # * Draw Class
  #     actor : actor
  #     x     : draw spot x-coordinate
  #     y     : draw spot y-coordinate
  #--------------------------------------------------------------------------
  def draw_actor_class(actor, x, y)
    self.contents.font.color = normal_color
    self.contents.draw_text(x, y, 236, 32, actor.class_name)
  end
  #--------------------------------------------------------------------------
  # * Draw Level
  #     actor : actor
  #     x     : draw spot x-coordinate
  #     y     : draw spot y-coordinate
  #--------------------------------------------------------------------------
  def draw_actor_level(actor, x, y)
    self.contents.font.color = system_color
    self.contents.draw_text(x, y, 32, 32, "Lv")
    self.contents.font.color = normal_color
    self.contents.draw_text(x + 32, y, 24, 32, actor.level.to_s, 2)
  end
  #--------------------------------------------------------------------------
  # * Make State Text String for Drawing
  #     actor       : actor
  #     width       : draw spot width
  #     need_normal : Whether or not [normal] is needed (true / false)
  #--------------------------------------------------------------------------
  def make_battler_state_text(battler, width, need_normal)
    # Get width of brackets
    brackets_width = self.contents.text_size("[]").width
    # Make text string for state names
    text = ""
    for i in battler.states
      if $data_states[i].rating >= 1
        if text == ""
          text = $data_states[i].name
        else
          new_text = text + "/" + $data_states[i].name
          text_width = self.contents.text_size(new_text).width
          if text_width > width - brackets_width
            break
          end
          text = new_text
        end
      end
    end
    # If text string for state names is empty, make it [normal]
    if text == ""
      if need_normal
        text = "[Normal]"
      end
    else
      # Attach brackets
      text = "[" + text + "]"
    end
    # Return completed text string
    return text
  end
  #--------------------------------------------------------------------------
  # * Draw State
  #     actor : actor
  #     x     : draw spot x-coordinate
  #     y     : draw spot y-coordinate
  #     width : draw spot width
  #--------------------------------------------------------------------------
  def draw_actor_state(actor, x, y, width = 120)
    text = make_battler_state_text(actor, width, true)
    self.contents.font.color = actor.hp == 0 ? knockout_color : normal_color
    self.contents.draw_text(x, y, width, 32, text)
  end
  #--------------------------------------------------------------------------
  # * Draw EXP
  #     actor : actor
  #     x     : draw spot x-coordinate
  #     y     : draw spot y-coordinate
  #--------------------------------------------------------------------------
  def draw_actor_exp(actor, x, y)
    self.contents.font.color = system_color
    self.contents.draw_text(x, y, 24, 32, "E")
    self.contents.font.color = normal_color
    self.contents.draw_text(x + 24, y, 84, 32, actor.exp_s, 2)
    self.contents.draw_text(x + 108, y, 12, 32, "/", 1)
    self.contents.draw_text(x + 120, y, 84, 32, actor.next_exp_s)
  end
  #--------------------------------------------------------------------------
  # * Draw HP
  #     actor : actor
  #     x     : draw spot x-coordinate
  #     y     : draw spot y-coordinate
  #     width : draw spot width
  #--------------------------------------------------------------------------
  def draw_actor_hp(actor, x, y, width = 144)
    # Draw "HP" text string
    self.contents.font.color = system_color
    self.contents.draw_text(x, y, 32, 32, $data_system.words.hp)
    # Calculate if there is draw space for MaxHP
    if width - 32 >= 108
      hp_x = x + width - 108
      flag = true
    elsif width - 32 >= 48
      hp_x = x + width - 48
      flag = false
    end
    # Draw HP
    self.contents.font.color = actor.hp == 0 ? knockout_color :
      actor.hp <= actor.maxhp / 4 ? crisis_color : normal_color
    self.contents.draw_text(hp_x, y, 48, 32, actor.hp.to_s, 2)
    # Draw MaxHP
    if flag
      self.contents.font.color = normal_color
      self.contents.draw_text(hp_x + 48, y, 12, 32, "/", 1)
      self.contents.draw_text(hp_x + 60, y, 48, 32, actor.maxhp.to_s)
    end
  end
  #--------------------------------------------------------------------------
  # * Draw SP
  #     actor : actor
  #     x     : draw spot x-coordinate
  #     y     : draw spot y-coordinate
  #     width : draw spot width
  #--------------------------------------------------------------------------
  def draw_actor_sp(actor, x, y, width = 144)
    # Draw "SP" text string
    self.contents.font.color = system_color
    self.contents.draw_text(x, y, 32, 32, $data_system.words.sp)
    # Calculate if there is draw space for MaxHP
    if width - 32 >= 108
      sp_x = x + width - 108
      flag = true
    elsif width - 32 >= 48
      sp_x = x + width - 48
      flag = false
    end
    # Draw SP
    self.contents.font.color = actor.sp == 0 ? knockout_color :
      actor.sp <= actor.maxsp / 4 ? crisis_color : normal_color
    self.contents.draw_text(sp_x, y, 48, 32, actor.sp.to_s, 2)
    # Draw MaxSP
    if flag
      self.contents.font.color = normal_color
      self.contents.draw_text(sp_x + 48, y, 12, 32, "/", 1)
      self.contents.draw_text(sp_x + 60, y, 48, 32, actor.maxsp.to_s)
    end
  end
  #--------------------------------------------------------------------------
  # * Draw Parameter
  #     actor : actor
  #     x     : draw spot x-coordinate
  #     y     : draw spot y-coordinate
  #     type  : parameter type (0-6)
  #--------------------------------------------------------------------------
  def draw_actor_parameter(actor, x, y, type)
    case type
    when 0
      parameter_name = $data_system.words.atk
      parameter_value = actor.atk
    when 1
      parameter_name = $data_system.words.pdef
      parameter_value = actor.pdef
    when 2
      parameter_name = $data_system.words.mdef
      parameter_value = actor.mdef
    when 3
      parameter_name = $data_system.words.str
      parameter_value = actor.str
    when 4
      parameter_name = $data_system.words.dex
      parameter_value = actor.dex
    when 5
      parameter_name = $data_system.words.agi
      parameter_value = actor.agi
    when 6
      parameter_name = $data_system.words.int
      parameter_value = actor.int
    end
    self.contents.font.color = system_color
    self.contents.draw_text(x, y, 120, 32, parameter_name)
    self.contents.font.color = normal_color
    self.contents.draw_text(x + 120, y, 36, 32, parameter_value.to_s, 2)
  end
  #--------------------------------------------------------------------------
  # * Draw Item Name
  #     item : item
  #     x    : draw spot x-coordinate
  #     y    : draw spot y-coordinate
  #--------------------------------------------------------------------------
  def draw_item_name(item, x, y)
    if item == nil
      return
    end
    bitmap = RPG::Cache.icon(item.icon_name)
    self.contents.blt(x, y + 4, bitmap, Rect.new(0, 0, 24, 24))
    self.contents.font.color = normal_color
    self.contents.draw_text(x + 28, y, 212, 32, item.name)
  end
end
