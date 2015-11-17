#==============================================================================
# ** Interpreter (part 3)
#------------------------------------------------------------------------------
#  This interpreter runs event commands. This class is used within the
#  Game_System class and the Game_Event class.
#==============================================================================

class Interpreter
  #--------------------------------------------------------------------------
  # * Show Text
  #--------------------------------------------------------------------------
  def command_101
    # If other text has been set to message_text
    if $game_temp.message_text != nil || $game_temp.message_ed_text != nil
      # End
      return false
    end

    # Create full text string
    choices = false
    text = @list[@index].parameters[0].strip
    loop do
      # 401: another line of text
      if @list[@index+1].code == 401
        @index += 1
        text << " " << @list[@index].parameters[0]
      else
        # 102: show choices
        if @list[@index+1].code == 102
          # Add choice data to message
          @index += 1
          setup_choices(@list[@index].parameters)
          choices = true
        # 103: choose number
        elsif @list[@index+1].code == 103
          # Add number data to message
          @index += 1
          $game_temp.num_input_variable_id = @list[@index].parameters[0]
          $game_temp.num_input_digits_max = @list[@index].parameters[1]
        end
        break
      end
    end
    text.gsub!(/\s+\\n\s+/, '\\n')
    text.strip!

    # Translate text
    text = $tr.event(@event_name, text).clone

    # Parse first line for portrait specifier
    if text[0,1] == '@'
      # Set portrait if text begins with @
      space = text.index(' ') || text.size
      face = text[1..space-1]
      if face == 'ed'
        is_ed = true
      else
        is_ed = false
        $game_temp.message_face = face
      end

      # Remove portrait text
      text.slice!(0..space)
    end

    # Set message end waiting flag and callback
    if !text.empty? || choices
      @message_waiting = true
      $game_temp.message_proc = Proc.new { @message_waiting = false }
    end

    # Set message box text
    unless text.empty?
      if is_ed
        $game_temp.message_ed_text = text
      else
        $game_temp.message_text = text
      end
    end
    return true
  end
  #--------------------------------------------------------------------------
  # * Show Choices
  #--------------------------------------------------------------------------
  def command_102
    # If text has been set to message_text
    if $game_temp.message_text != nil
      # End
      return false
    end
    # Set message end waiting flag and callback
    @message_waiting = true
    $game_temp.message_proc = Proc.new { @message_waiting = false }
    # Choices setup
    setup_choices(@parameters)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * When [**]
  #--------------------------------------------------------------------------
  def command_402
    # If fitting choices are selected
    if @branch[@list[@index].indent] == @parameters[0]
      # Delete branch data
      @branch.delete(@list[@index].indent)
      # Continue
      return true
    end
    # If it doesn't meet the condition: command skip
    return command_skip
  end
  #--------------------------------------------------------------------------
  # * When Cancel
  #--------------------------------------------------------------------------
  def command_403
    # If choices are cancelled
    if @branch[@list[@index].indent] == 4
      # Delete branch data
      @branch.delete(@list[@index].indent)
      # Continue
      return true
    end
    # If it doen't meet the condition: command skip
    return command_skip
  end
  #--------------------------------------------------------------------------
  # * Input Number
  #--------------------------------------------------------------------------
  def command_103
    # If text has been set to message_text
    if $game_temp.message_text != nil
      # End
      return false
    end
    # Set message end waiting flag and callback
    @message_waiting = true
    $game_temp.message_proc = Proc.new { @message_waiting = false }
    # Number input setup
    $game_temp.num_input_variable_id = @parameters[0]
    $game_temp.num_input_digits_max = @parameters[1]
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Text Options
  #--------------------------------------------------------------------------
  def command_104
    # If message is showing
    if $game_temp.message_window_showing
      # End
      return false
    end
    # Change each option
    $game_system.message_position = @parameters[0]
    $game_system.message_frame = @parameters[1]
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Button Input Processing
  #--------------------------------------------------------------------------
  def command_105
    # Set variable ID for button input
    @button_input_variable_id = @parameters[0]
    # Advance index
    @index += 1
    # End
    return false
  end
  #--------------------------------------------------------------------------
  # * Wait
  #--------------------------------------------------------------------------
  def command_106
    # Set wait count
    @wait_count = @parameters[0] * 2
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Conditional Branch
  #--------------------------------------------------------------------------
  def command_111
    # Initialize local variable: result
    result = false
    case @parameters[0]
    when 0  # switch
      result = ($game_switches[@parameters[1]] == (@parameters[2] == 0))
    when 1  # variable
      value1 = $game_variables[@parameters[1]]
      if @parameters[2] == 0
        value2 = @parameters[3]
      else
        value2 = $game_variables[@parameters[3]]
      end
      case @parameters[4]
      when 0  # value1 is equal to value2
        result = (value1 == value2)
      when 1  # value1 is greater than or equal to value2
        result = (value1 >= value2)
      when 2  # value1 is less than or equal to value2
        result = (value1 <= value2)
      when 3  # value1 is greater than value2
        result = (value1 > value2)
      when 4  # value1 is less than value2
        result = (value1 < value2)
      when 5  # value1 is not equal to value2
        result = (value1 != value2)
      end
    when 2  # self switch
      if @event_id > 0
        key = [$game_map.map_id, @event_id, @parameters[1]]
        if @parameters[2] == 0
          result = ($game_self_switches[key] == true)
        else
          result = ($game_self_switches[key] != true)
        end
      end
    when 3  # timer
      if $game_system.timer_working
        sec = $game_system.timer / Graphics.frame_rate
        if @parameters[2] == 0
          result = (sec >= @parameters[1])
        else
          result = (sec <= @parameters[1])
        end
      end
    when 4  # actor
      actor = $game_actors[@parameters[1]]
      if actor != nil
        case @parameters[2]
        when 0  # in party
          result = ($game_party.actors.include?(actor))
        when 1  # name
          result = (actor.name == @parameters[3])
        when 2  # skill
          result = (actor.skill_learn?(@parameters[3]))
        when 3  # weapon
          result = (actor.weapon_id == @parameters[3])
        when 4  # armor
          result = (actor.armor1_id == @parameters[3] or
                    actor.armor2_id == @parameters[3] or
                    actor.armor3_id == @parameters[3] or
                    actor.armor4_id == @parameters[3])
        when 5  # state
          result = (actor.state?(@parameters[3]))
        end
      end
    when 5  # enemy
      enemy = $game_troop.enemies[@parameters[1]]
      if enemy != nil
        case @parameters[2]
        when 0  # appear
          result = (enemy.exist?)
        when 1  # state
          result = (enemy.state?(@parameters[3]))
        end
      end
    when 6  # character
      character = get_character(@parameters[1])
      if character != nil
        result = (character.direction == @parameters[2])
      end
    when 7  # gold
      if @parameters[2] == 0
        result = ($game_party.gold >= @parameters[1])
      else
        result = ($game_party.gold <= @parameters[1])
      end
    when 8  # item
      result = ($game_party.item_number(@parameters[1]) > 0)
    when 9  # weapon
      result = ($game_party.weapon_number(@parameters[1]) > 0)
    when 10  # armor
      result = ($game_party.armor_number(@parameters[1]) > 0)
    when 11  # button
      result = (Input.press?(@parameters[1]))
    when 12  # script
      result = eval(@parameters[1])
    end
    # Store determinant results in hash
    @branch[@list[@index].indent] = result
    # If determinant results are true
    if @branch[@list[@index].indent] == true
      # Delete branch data
      @branch.delete(@list[@index].indent)
      # Continue
      return true
    end
    # If it doesn't meet the conditions: command skip
    return command_skip
  end
  #--------------------------------------------------------------------------
  # * Else
  #--------------------------------------------------------------------------
  def command_411
    # If determinant results are false
    if @branch[@list[@index].indent] == false
      # Delete branch data
      @branch.delete(@list[@index].indent)
      # Continue
      return true
    end
    # If it doesn't meet the conditions: command skip
    return command_skip
  end
  #--------------------------------------------------------------------------
  # * Loop
  #--------------------------------------------------------------------------
  def command_112
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Repeat Above
  #--------------------------------------------------------------------------
  def command_413
    # Get indent
    indent = @list[@index].indent
    # Loop
    loop do
      # Return index
      @index -= 1
      # If this event command is the same level as indent
      if @list[@index].indent == indent
        # Continue
        return true
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Break Loop
  #--------------------------------------------------------------------------
  def command_113
    # Get indent
    indent = @list[@index].indent
    # Copy index to temporary variables
    temp_index = @index
    # Loop
    loop do
      # Advance index
      temp_index += 1
      # If a fitting loop was not found
      if temp_index >= @list.size-1
        # Continue
        return true
      end
      # If this event command is [repeat above] and indent is shallow
      if @list[temp_index].code == 413 and @list[temp_index].indent < indent
        # Update index
        @index = temp_index
        # Continue
        return true
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Exit Event Processing
  #--------------------------------------------------------------------------
  def command_115
    # End event
    command_end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Erase Event
  #--------------------------------------------------------------------------
  def command_116
    # If event ID is valid
    if @event_id > 0
      # Erase event
      $game_map.events[@event_id].erase
    end
    # Advance index
    @index += 1
    # End
    return false
  end
  #--------------------------------------------------------------------------
  # * Call Common Event
  #--------------------------------------------------------------------------
  def command_117
    # Get common event
    common_event = $data_common_events[@parameters[0]]
    # If common event is valid
    if common_event != nil
      # Make child interpreter
      @child_interpreter = Interpreter.new(@depth + 1)
      @child_interpreter.setup(common_event.list, @event_id, common_event.name)
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Label
  #--------------------------------------------------------------------------
  def command_118
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Jump to Label
  #--------------------------------------------------------------------------
  def command_119
    # Get label name
    label_name = @parameters[0]
    # Initialize temporary variables
    temp_index = 0
    # Loop
    loop do
      # If a fitting label was not found
      if temp_index >= @list.size-1
        # Continue
        return true
      end
      # If this event command is a designated label name
      if @list[temp_index].code == 118 and
         @list[temp_index].parameters[0] == label_name
        # Update index
        @index = temp_index
        # Continue
        return true
      end
      # Advance index
      temp_index += 1
    end
  end
end
