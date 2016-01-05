#==============================================================================
# ** Interpreter (part 6)
#------------------------------------------------------------------------------
#  This interpreter runs event commands. This class is used within the
#  Game_System class and the Game_Event class.
#==============================================================================

class Interpreter
  #--------------------------------------------------------------------------
  # * Battle Processing
  #--------------------------------------------------------------------------
  def command_301
    # If not invalid troops
    if $data_troops[@parameters[0]] != nil
      # Set battle abort flag
      $game_temp.battle_abort = true
      # Set battle calling flag
      $game_temp.battle_calling = true
      $game_temp.battle_troop_id = @parameters[0]
      $game_temp.battle_can_escape = @parameters[1]
      $game_temp.battle_can_lose = @parameters[2]
      # Set callback
      current_indent = @list[@index].indent
      $game_temp.battle_proc = Proc.new { |n| @branch[current_indent] = n }
    end
    # Advance index
    @index += 1
    # End
    return false
  end
  #--------------------------------------------------------------------------
  # * If Win
  #--------------------------------------------------------------------------
  def command_601
    # When battle results = win
    if @branch[@list[@index].indent] == 0
      # Delete branch data
      @branch.delete(@list[@index].indent)
      # Continue
      return true
    end
    # If it doesn't meet conditions: command skip
    return command_skip
  end
  #--------------------------------------------------------------------------
  # * If Escape
  #--------------------------------------------------------------------------
  def command_602
    # If battle results = escape
    if @branch[@list[@index].indent] == 1
      # Delete branch data
      @branch.delete(@list[@index].indent)
      # Continue
      return true
    end
    # If it doesn't meet conditions: command skip
    return command_skip
  end
  #--------------------------------------------------------------------------
  # * If Lose
  #--------------------------------------------------------------------------
  def command_603
    # If battle results = lose
    if @branch[@list[@index].indent] == 2
      # Delete branch data
      @branch.delete(@list[@index].indent)
      # Continue
      return true
    end
    # If it doesn't meet conditions: command skip
    return command_skip
  end
  #--------------------------------------------------------------------------
  # * Shop Processing
  #--------------------------------------------------------------------------
  def command_302
    # Set battle abort flag
    $game_temp.battle_abort = true
    # Set shop calling flag
    $game_temp.shop_calling = true
    # Set goods list on new item
    $game_temp.shop_goods = [@parameters]
    # Loop
    loop do
      # Advance index
      @index += 1
      # If next event command has shop on second line or after
      if @list[@index].code == 605
        # Add goods list to new item
        $game_temp.shop_goods.push(@list[@index].parameters)
      # If event command does not have shop on second line or after
      else
        # End
        return false
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Name Input Processing
  #--------------------------------------------------------------------------
  def command_303
    # If not invalid actors
    if $data_actors[@parameters[0]] != nil
      # Set battle abort flag
      $game_temp.battle_abort = true
      # Set name input calling flag
      $game_temp.name_calling = true
    end
    # Advance index
    @index += 1
    # End
    return false
  end
  #--------------------------------------------------------------------------
  # * Change HP
  #--------------------------------------------------------------------------
  def command_311
    # Get operate value
    value = operate_value(@parameters[1], @parameters[2], @parameters[3])
    # Process with iterator
    iterate_actor(@parameters[0]) do |actor|
      # If HP are not 0
      if actor.hp > 0
        # Change HP (if death is not permitted, make HP 1)
        if @parameters[4] == false and actor.hp + value <= 0
          actor.hp = 1
        else
          actor.hp += value
        end
      end
    end
    # Determine game over
    $game_temp.gameover = $game_party.all_dead?
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change SP
  #--------------------------------------------------------------------------
  def command_312
    # Get operate value
    value = operate_value(@parameters[1], @parameters[2], @parameters[3])
    # Process with iterator
    iterate_actor(@parameters[0]) do |actor|
      # Change actor SP
      actor.sp += value
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change State
  #--------------------------------------------------------------------------
  def command_313
    # Process with iterator
    iterate_actor(@parameters[0]) do |actor|
      # Change state
      if @parameters[1] == 0
        actor.add_state(@parameters[2])
      else
        actor.remove_state(@parameters[2])
      end
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Recover All
  #--------------------------------------------------------------------------
  def command_314
    # Process with iterator
    iterate_actor(@parameters[0]) do |actor|
      # Recover all for actor
      actor.recover_all
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change EXP
  #--------------------------------------------------------------------------
  def command_315
    # Get operate value
    value = operate_value(@parameters[1], @parameters[2], @parameters[3])
    # Process with iterator
    iterate_actor(@parameters[0]) do |actor|
      # Change actor EXP
      actor.exp += value
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Level
  #--------------------------------------------------------------------------
  def command_316
    # Get operate value
    value = operate_value(@parameters[1], @parameters[2], @parameters[3])
    # Process with iterator
    iterate_actor(@parameters[0]) do |actor|
      # Change actor level
      actor.level += value
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Parameters
  #--------------------------------------------------------------------------
  def command_317
    # Get operate value
    value = operate_value(@parameters[2], @parameters[3], @parameters[4])
    # Get actor
    actor = $game_actors[@parameters[0]]
    # Change parameters
    if actor != nil
      case @parameters[1]
      when 0  # MaxHP
        actor.maxhp += value
      when 1  # MaxSP
        actor.maxsp += value
      when 2  # strength
        actor.str += value
      when 3  # dexterity
        actor.dex += value
      when 4  # agility
        actor.agi += value
      when 5  # intelligence
        actor.int += value
      end
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Skills
  #--------------------------------------------------------------------------
  def command_318
    # Get actor
    actor = $game_actors[@parameters[0]]
    # Change skill
    if actor != nil
      if @parameters[1] == 0
        actor.learn_skill(@parameters[2])
      else
        actor.forget_skill(@parameters[2])
      end
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Equipment
  #--------------------------------------------------------------------------
  def command_319
    # Get actor
    actor = $game_actors[@parameters[0]]
    # Change Equipment
    if actor != nil
      actor.equip(@parameters[1], @parameters[2])
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Actor Name
  #--------------------------------------------------------------------------
  def command_320
    # Get actor
    actor = $game_actors[@parameters[0]]
    # Change name
    if actor != nil
      actor.name = @parameters[1]
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Actor Class
  #--------------------------------------------------------------------------
  def command_321
    # Get actor
    actor = $game_actors[@parameters[0]]
    # Change class
    if actor != nil
      actor.class_id = @parameters[1]
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Actor Graphic
  #--------------------------------------------------------------------------
  def command_322
    # Get actor
    actor = $game_actors[@parameters[0]]
    # Change graphic
    if actor != nil
      actor.set_graphic(@parameters[1], @parameters[2],
        @parameters[3], @parameters[4])
    end
    # Refresh player
    $game_player.refresh
    # Continue
    return true
  end
end
