#==============================================================================
# ** Scene_Battle (part 3)
#------------------------------------------------------------------------------
#  This class performs battle screen processing.
#==============================================================================

class Scene_Battle
  #--------------------------------------------------------------------------
  # * Start Actor Command Phase
  #--------------------------------------------------------------------------
  def start_phase3
    # Shift to phase 3
    @phase = 3
    # Set actor as unselectable
    @actor_index = -1
    @active_battler = nil
    # Go to command input for next actor
    phase3_next_actor
  end
  #--------------------------------------------------------------------------
  # * Go to Command Input for Next Actor
  #--------------------------------------------------------------------------
  def phase3_next_actor
    # Loop
    begin
      # Actor blink effect OFF
      if @active_battler != nil
        @active_battler.blink = false
      end
      # If last actor
      if @actor_index == $game_party.actors.size-1
        # Start main phase
        start_phase4
        return
      end
      # Advance actor index
      @actor_index += 1
      @active_battler = $game_party.actors[@actor_index]
      @active_battler.blink = true
    # Once more if actor refuses command input
    end until @active_battler.inputable?
    # Set up actor command window
    phase3_setup_command_window
  end
  #--------------------------------------------------------------------------
  # * Go to Command Input of Previous Actor
  #--------------------------------------------------------------------------
  def phase3_prior_actor
    # Loop
    begin
      # Actor blink effect OFF
      if @active_battler != nil
        @active_battler.blink = false
      end
      # If first actor
      if @actor_index == 0
        # Start party command phase
        start_phase2
        return
      end
      # Return to actor index
      @actor_index -= 1
      @active_battler = $game_party.actors[@actor_index]
      @active_battler.blink = true
    # Once more if actor refuses command input
    end until @active_battler.inputable?
    # Set up actor command window
    phase3_setup_command_window
  end
  #--------------------------------------------------------------------------
  # * Actor Command Window Setup
  #--------------------------------------------------------------------------
  def phase3_setup_command_window
    # Disable party command window
    @party_command_window.active = false
    @party_command_window.visible = false
    # Enable actor command window
    @actor_command_window.active = true
    @actor_command_window.visible = true
    # Set actor command window position
    @actor_command_window.x = @actor_index * 160
    # Set index to 0
    @actor_command_window.index = 0
  end
  #--------------------------------------------------------------------------
  # * Frame Update (actor command phase)
  #--------------------------------------------------------------------------
  def update_phase3
    # If enemy arrow is enabled
    if @enemy_arrow != nil
      update_phase3_enemy_select
    # If actor arrow is enabled
    elsif @actor_arrow != nil
      update_phase3_actor_select
    # If skill window is enabled
    elsif @skill_window != nil
      update_phase3_skill_select
    # If item window is enabled
    elsif @item_window != nil
      update_phase3_item_select
    # If actor command window is enabled
    elsif @actor_command_window.active
      update_phase3_basic_command
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (actor command phase : basic command)
  #--------------------------------------------------------------------------
  def update_phase3_basic_command
    # If B button was pressed
    if Input.trigger?(Input::CANCEL)
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # Go to command input for previous actor
      phase3_prior_actor
      return
    end
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # Branch by actor command window cursor position
      case @actor_command_window.index
      when 0  # attack
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Set action
        @active_battler.current_action.kind = 0
        @active_battler.current_action.basic = 0
        # Start enemy selection
        start_enemy_select
      when 1  # skill
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Set action
        @active_battler.current_action.kind = 1
        # Start skill selection
        start_skill_select
      when 2  # guard
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Set action
        @active_battler.current_action.kind = 0
        @active_battler.current_action.basic = 1
        # Go to command input for next actor
        phase3_next_actor
      when 3  # item
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Set action
        @active_battler.current_action.kind = 2
        # Start item selection
        start_item_select
      end
      return
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (actor command phase : skill selection)
  #--------------------------------------------------------------------------
  def update_phase3_skill_select
    # Make skill window visible
    @skill_window.visible = true
    # Update skill window
    @skill_window.update
    # If B button was pressed
    if Input.trigger?(Input::CANCEL)
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # End skill selection
      end_skill_select
      return
    end
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # Get currently selected data on the skill window
      @skill = @skill_window.skill
      # If it can't be used
      if @skill == nil or not @active_battler.skill_can_use?(@skill.id)
        # Play buzzer SE
        $game_system.se_play($data_system.buzzer_se)
        return
      end
      # Play decision SE
      $game_system.se_play($data_system.decision_se)
      # Set action
      @active_battler.current_action.skill_id = @skill.id
      # Make skill window invisible
      @skill_window.visible = false
      # If effect scope is single enemy
      if @skill.scope == 1
        # Start enemy selection
        start_enemy_select
      # If effect scope is single ally
      elsif @skill.scope == 3 or @skill.scope == 5
        # Start actor selection
        start_actor_select
      # If effect scope is not single
      else
        # End skill selection
        end_skill_select
        # Go to command input for next actor
        phase3_next_actor
      end
      return
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (actor command phase : item selection)
  #--------------------------------------------------------------------------
  def update_phase3_item_select
    # Make item window visible
    @item_window.visible = true
    # Update item window
    @item_window.update
    # If B button was pressed
    if Input.trigger?(Input::CANCEL)
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # End item selection
      end_item_select
      return
    end
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # Get currently selected data on the item window
      @item = @item_window.item
      # If it can't be used
      unless $game_party.item_can_use?(@item.id)
        # Play buzzer SE
        $game_system.se_play($data_system.buzzer_se)
        return
      end
      # Play decision SE
      $game_system.se_play($data_system.decision_se)
      # Set action
      @active_battler.current_action.item_id = @item.id
      # Make item window invisible
      @item_window.visible = false
      # If effect scope is single enemy
      if @item.scope == 1
        # Start enemy selection
        start_enemy_select
      # If effect scope is single ally
      elsif @item.scope == 3 or @item.scope == 5
        # Start actor selection
        start_actor_select
      # If effect scope is not single
      else
        # End item selection
        end_item_select
        # Go to command input for next actor
        phase3_next_actor
      end
      return
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Updat (actor command phase : enemy selection)
  #--------------------------------------------------------------------------
  def update_phase3_enemy_select
    # Update enemy arrow
    @enemy_arrow.update
    # If B button was pressed
    if Input.trigger?(Input::CANCEL)
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # End enemy selection
      end_enemy_select
      return
    end
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # Play decision SE
      $game_system.se_play($data_system.decision_se)
      # Set action
      @active_battler.current_action.target_index = @enemy_arrow.index
      # End enemy selection
      end_enemy_select
      # If skill window is showing
      if @skill_window != nil
        # End skill selection
        end_skill_select
      end
      # If item window is showing
      if @item_window != nil
        # End item selection
        end_item_select
      end
      # Go to command input for next actor
      phase3_next_actor
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (actor command phase : actor selection)
  #--------------------------------------------------------------------------
  def update_phase3_actor_select
    # Update actor arrow
    @actor_arrow.update
    # If B button was pressed
    if Input.trigger?(Input::CANCEL)
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # End actor selection
      end_actor_select
      return
    end
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # Play decision SE
      $game_system.se_play($data_system.decision_se)
      # Set action
      @active_battler.current_action.target_index = @actor_arrow.index
      # End actor selection
      end_actor_select
      # If skill window is showing
      if @skill_window != nil
        # End skill selection
        end_skill_select
      end
      # If item window is showing
      if @item_window != nil
        # End item selection
        end_item_select
      end
      # Go to command input for next actor
      phase3_next_actor
    end
  end
  #--------------------------------------------------------------------------
  # * Start Enemy Selection
  #--------------------------------------------------------------------------
  def start_enemy_select
    # Make enemy arrow
    @enemy_arrow = Arrow_Enemy.new(@spriteset.viewport1)
    # Associate help window
    @enemy_arrow.help_window = @help_window
    # Disable actor command window
    @actor_command_window.active = false
    @actor_command_window.visible = false
  end
  #--------------------------------------------------------------------------
  # * End Enemy Selection
  #--------------------------------------------------------------------------
  def end_enemy_select
    # Dispose of enemy arrow
    @enemy_arrow.dispose
    @enemy_arrow = nil
    # If command is [fight]
    if @actor_command_window.index == 0
      # Enable actor command window
      @actor_command_window.active = true
      @actor_command_window.visible = true
      # Hide help window
      @help_window.visible = false
    end
  end
  #--------------------------------------------------------------------------
  # * Start Actor Selection
  #--------------------------------------------------------------------------
  def start_actor_select
    # Make actor arrow
    @actor_arrow = Arrow_Actor.new(@spriteset.viewport2)
    @actor_arrow.index = @actor_index
    # Associate help window
    @actor_arrow.help_window = @help_window
    # Disable actor command window
    @actor_command_window.active = false
    @actor_command_window.visible = false
  end
  #--------------------------------------------------------------------------
  # * End Actor Selection
  #--------------------------------------------------------------------------
  def end_actor_select
    # Dispose of actor arrow
    @actor_arrow.dispose
    @actor_arrow = nil
  end
  #--------------------------------------------------------------------------
  # * Start Skill Selection
  #--------------------------------------------------------------------------
  def start_skill_select
    # Make skill window
    @skill_window = Window_Skill.new(@active_battler)
    # Associate help window
    @skill_window.help_window = @help_window
    # Disable actor command window
    @actor_command_window.active = false
    @actor_command_window.visible = false
  end
  #--------------------------------------------------------------------------
  # * End Skill Selection
  #--------------------------------------------------------------------------
  def end_skill_select
    # Dispose of skill window
    @skill_window.dispose
    @skill_window = nil
    # Hide help window
    @help_window.visible = false
    # Enable actor command window
    @actor_command_window.active = true
    @actor_command_window.visible = true
  end
  #--------------------------------------------------------------------------
  # * Start Item Selection
  #--------------------------------------------------------------------------
  def start_item_select
    # Make item window
    @item_window = Window_Item.new
    # Associate help window
    @item_window.help_window = @help_window
    # Disable actor command window
    @actor_command_window.active = false
    @actor_command_window.visible = false
  end
  #--------------------------------------------------------------------------
  # * End Item Selection
  #--------------------------------------------------------------------------
  def end_item_select
    # Dispose of item window
    @item_window.dispose
    @item_window = nil
    # Hide help window
    @help_window.visible = false
    # Enable actor command window
    @actor_command_window.active = true
    @actor_command_window.visible = true
  end
end
