#==============================================================================
# ** Scene_Menu
#------------------------------------------------------------------------------
#  This class performs menu screen processing.
#==============================================================================

class Scene_Menu
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     menu_index : command cursor's initial position
  #--------------------------------------------------------------------------
  def initialize(menu_index = 0)
    @menu_index = menu_index
  end
  #--------------------------------------------------------------------------
  # * Main Processing
  #--------------------------------------------------------------------------
  def main
    # Make command window
    s1 = $data_system.words.item
    s2 = $data_system.words.skill
    s3 = $data_system.words.equip
    s4 = "Status"
    s5 = "Save"
    s6 = "End Game"
    @command_window = Window_Command.new(160, [s1, s2, s3, s4, s5, s6])
    @command_window.index = @menu_index
    # If number of party members is 0
    if $game_party.actors.size == 0
      # Disable items, skills, equipment, and status
      @command_window.disable_item(0)
      @command_window.disable_item(1)
      @command_window.disable_item(2)
      @command_window.disable_item(3)
    end
    # If save is forbidden
    if $game_system.save_disabled
      # Disable save
      @command_window.disable_item(4)
    end
    # Make play time window
    @playtime_window = Window_PlayTime.new
    @playtime_window.x = 0
    @playtime_window.y = 224
    # Make steps window
    @steps_window = Window_Steps.new
    @steps_window.x = 0
    @steps_window.y = 320
    # Make gold window
    @gold_window = Window_Gold.new
    @gold_window.x = 0
    @gold_window.y = 416
    # Make status window
    @status_window = Window_MenuStatus.new
    @status_window.x = 160
    @status_window.y = 0
    # Execute transition
    Graphics.transition
    # Main loop
    loop do
      # Update game screen
      Graphics.update
      # Update input information
      Input.update
      # Frame update
      update
      # Abort loop if screen is changed
      if $scene != self
        break
      end
    end
    # Prepare for transition
    Graphics.freeze
    # Dispose of windows
    @command_window.dispose
    @playtime_window.dispose
    @steps_window.dispose
    @gold_window.dispose
    @status_window.dispose
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    # Update windows
    @command_window.update
    @playtime_window.update
    @steps_window.update
    @gold_window.update
    @status_window.update
    # If command window is active: call update_command
    if @command_window.active
      update_command
      return
    end
    # If status window is active: call update_status
    if @status_window.active
      update_status
      return
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (when command window is active)
  #--------------------------------------------------------------------------
  def update_command
    # If B button was pressed
    if Input.trigger?(Input::CANCEL)
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # Switch to map screen
      $scene = Scene_Map.new
      return
    end
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # If command other than save or end game, and party members = 0
      if $game_party.actors.size == 0 and @command_window.index < 4
        # Play buzzer SE
        $game_system.se_play($data_system.buzzer_se)
        return
      end
      # Branch by command window cursor position
      case @command_window.index
      when 0  # item
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Switch to item screen
        $scene = Scene_Item.new
      when 1  # skill
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Make status window active
        @command_window.active = false
        @status_window.active = true
        @status_window.index = 0
      when 2  # equipment
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Make status window active
        @command_window.active = false
        @status_window.active = true
        @status_window.index = 0
      when 3  # status
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Make status window active
        @command_window.active = false
        @status_window.active = true
        @status_window.index = 0
      when 4  # save
        # If saving is forbidden
        if $game_system.save_disabled
          # Play buzzer SE
          $game_system.se_play($data_system.buzzer_se)
          return
        end
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Switch to save screen
        $scene = Scene_Save.new
      when 5  # end game
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Switch to end game screen
        $scene = Scene_End.new
      end
      return
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (when status window is active)
  #--------------------------------------------------------------------------
  def update_status
    # If B button was pressed
    if Input.trigger?(Input::CANCEL)
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # Make command window active
      @command_window.active = true
      @status_window.active = false
      @status_window.index = -1
      return
    end
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # Branch by command window cursor position
      case @command_window.index
      when 1  # skill
        # If this actor's action limit is 2 or more
        if $game_party.actors[@status_window.index].restriction >= 2
          # Play buzzer SE
          $game_system.se_play($data_system.buzzer_se)
          return
        end
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Switch to skill screen
        $scene = Scene_Skill.new(@status_window.index)
      when 2  # equipment
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Switch to equipment screen
        $scene = Scene_Equip.new(@status_window.index)
      when 3  # status
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Switch to status screen
        $scene = Scene_Status.new(@status_window.index)
      end
      return
    end
  end
end
