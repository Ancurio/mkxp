#==============================================================================
# ** Interpreter (part 1)
#------------------------------------------------------------------------------
#  This interpreter runs event commands. This class is used within the
#  Game_System class and the Game_Event class.
#==============================================================================

class Interpreter
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     depth : nest depth
  #     main  : main flag
  #--------------------------------------------------------------------------
  def initialize(depth = 0, main = false)
    @depth = depth
    @main = main
    # Depth goes up to level 100
    if depth > 100
      print("Common event call has exceeded maximum limit.")
      exit
    end
    # Clear inner situation of interpreter
    clear
  end
  #--------------------------------------------------------------------------
  # * Clear
  #--------------------------------------------------------------------------
  def clear
    @map_id = 0                       # map ID when starting up
    @event_id = 0                     # event ID
    @message_waiting = false          # waiting for message to end
    @move_route_waiting = false       # waiting for move completion
    @button_input_variable_id = 0     # button input variable ID
    @wait_count = 0                   # wait count
    @child_interpreter = nil          # child interpreter
    @branch = {}                      # branch data
    @event_name = nil                 # full event name
  end
  #--------------------------------------------------------------------------
  # * Event Setup
  #     list     : list of event commands
  #     event_id : event ID
  #--------------------------------------------------------------------------
  def setup(list, event_id, common_event_name = nil)
    # Clear inner situation of interpreter
    clear
    # Remember map ID
    @map_id = $game_map.map_id
    # Remember event ID
    @event_id = event_id
    # Remember list of event commands
    @list = list
    # Initialize index
    @index = 0
    # Clear branch data hash
    @branch.clear
    # Construct event name
    if common_event_name == nil
      if event_id != 0
        @event_name = $game_map.map_name + "/" + $game_map.events[event_id].name
      end
    else
      @event_name = "/" + common_event_name
    end
  end
  #--------------------------------------------------------------------------
  # * Determine if Running
  #--------------------------------------------------------------------------
  def running?
    return @list != nil
  end
  #--------------------------------------------------------------------------
  # * Starting Event Setup
  #--------------------------------------------------------------------------
  def setup_starting_event
    # Refresh map if necessary
    if $game_map.need_refresh
      $game_map.refresh
    end
    # If common event call is reserved
    if $game_temp.common_event_id > 0
      # Set up event
      common_event = $data_common_events[$game_temp.common_event_id]
      setup(common_event.list, 0, common_event.name)
      # Release reservation
      $game_temp.common_event_id = 0
      return
    end
    # Loop (map events)
    for event in $game_map.events.values
      # If running event is found
      if event.starting
        # If not auto run
        if event.trigger < 3
          # Clear starting flag
          event.clear_starting
          # Lock
          event.lock
        end
        # Set up event
        setup(event.list, event.id)
        return
      end
    end
    # Loop (common events)
    for common_event in $data_common_events.compact
      # If trigger is auto run, and condition switch is ON
      if common_event.trigger == 1 and
         $game_switches[common_event.switch_id] == true
        # Set up event
        setup(common_event.list, 0, common_event.name)
        return
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    # Initialize loop count
    @loop_count = 0
    # Loop
    loop do
      # Add 1 to loop count
      @loop_count += 1
      # If 100 event commands ran
      if @loop_count > 100
        # Call Graphics.update for freeze prevention
        Graphics.update
        @loop_count = 0
      end
      # If map is different than event startup time
      if $game_map.map_id != @map_id
        # Change event ID to 0
        @event_id = 0
      end
      # If a child interpreter exists
      if @child_interpreter != nil
        # Update child interpreter
        @child_interpreter.update
        # If child interpreter is finished running
        unless @child_interpreter.running?
          # Delete child interpreter
          @child_interpreter = nil
        end
        # If child interpreter still exists
        if @child_interpreter != nil
          return
        end
      end
      # If waiting for message to end
      if @message_waiting
        return
      end
      # If waiting for move to end
      if @move_route_waiting
        # If player is forcing move route
        if $game_player.move_route_forcing
          return
        end
        # Loop (map events)
        for event in $game_map.events.values
          # If this event is forcing move route
          if event.move_route_forcing
            return
          end
        end
        # Clear move end waiting flag
        @move_route_waiting = false
      end
      # If waiting for button input
      if @button_input_variable_id > 0
        # Run button input processing
        input_button
        return
      end
      # If waiting
      if @wait_count > 0
        # Decrease wait count
        @wait_count -= 1
        return
      end
      # If a call flag is set for each type of screen
      if $game_temp.name_calling
        return
      end
      # If list of event commands is empty
      if @list == nil
        # If main map event
        if @main
          # Set up starting event
          setup_starting_event
        end
        # If nothing was set up
        if @list == nil
          return
        end
      end
      # If return value is false when trying to execute event command
      if execute_command == false
        return
      end
      # Advance index
      @index += 1
    end
  end
  #--------------------------------------------------------------------------
  # * Button Input
  #--------------------------------------------------------------------------
  def input_button
    # Determine pressed button
    n = 0
    for i in 1..18
      if Input.trigger?(i)
        n = i
      end
    end
    # If button was pressed
    if n > 0
      # Change value of variables
      $game_variables[@button_input_variable_id] = n
      $game_map.need_refresh = true
      # End button input
      @button_input_variable_id = 0
    end
  end
  #--------------------------------------------------------------------------
  # * Setup Choices
  #--------------------------------------------------------------------------
  def setup_choices(parameters)
    # Set choices
    $game_temp.choices = parameters[0].map{|s| $tr.event(@event_name, s.strip)}
    # Set cancel processing
    $game_temp.choice_cancel_type = parameters[1]
    # Set callback
    current_indent = @list[@index].indent
    $game_temp.choice_proc = Proc.new { |n| @branch[current_indent] = n }
  end
  #--------------------------------------------------------------------------
  # * Actor Iterator (consider all party members)
  #     parameter : if 1 or more, ID; if 0, all
  #--------------------------------------------------------------------------
  def iterate_actor(parameter)
    # If entire party
    if parameter == 0
      # Loop for entire party
      for actor in $game_party.actors
        # Evaluate block
        yield actor
      end
    # If single actor
    else
      # Get actor
      actor = $game_actors[parameter]
      # Evaluate block
      yield actor if actor != nil
    end
  end
  #--------------------------------------------------------------------------
  # * Enemy Iterator (consider all troop members)
  #     parameter : If 0 or above, index; if -1, all
  #--------------------------------------------------------------------------
  def iterate_enemy(parameter)
    # If entire troop
    if parameter == -1
      # Loop for entire troop
      for enemy in $game_troop.enemies
        # Evaluate block
        yield enemy
      end
    # If single enemy
    else
      # Get enemy
      enemy = $game_troop.enemies[parameter]
      # Evaluate block
      yield enemy if enemy != nil
    end
  end
  #--------------------------------------------------------------------------
  # * Battler Iterator (consider entire troop and entire party)
  #     parameter1 : If 0, enemy; if 1, actor
  #     parameter2 : If 0 or above, index; if -1, all
  #--------------------------------------------------------------------------
  def iterate_battler(parameter1, parameter2)
    # If enemy
    if parameter1 == 0
      # Call enemy iterator
      iterate_enemy(parameter2) do |enemy|
        yield enemy
      end
    # If actor
    else
      # If entire party
      if parameter2 == -1
        # Loop for entire party
        for actor in $game_party.actors
          # Evaluate block
          yield actor
        end
      # If single actor (N exposed)
      else
        # Get actor
        actor = $game_party.actors[parameter2]
        # Evaluate block
        yield actor if actor != nil
      end
    end
  end
end
