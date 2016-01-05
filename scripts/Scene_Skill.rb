#==============================================================================
# ** Scene_Skill
#------------------------------------------------------------------------------
#  This class performs skill screen processing.
#==============================================================================

class Scene_Skill
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     actor_index : actor index
  #--------------------------------------------------------------------------
  def initialize(actor_index = 0, equip_index = 0)
    @actor_index = actor_index
  end
  #--------------------------------------------------------------------------
  # * Main Processing
  #--------------------------------------------------------------------------
  def main
    # Get actor
    @actor = $game_party.actors[@actor_index]
    # Make help window, status window, and skill window
    @help_window = Window_Help.new
    @status_window = Window_SkillStatus.new(@actor)
    @skill_window = Window_Skill.new(@actor)
    # Associate help window
    @skill_window.help_window = @help_window
    # Make target window (set to invisible / inactive)
    @target_window = Window_Target.new
    @target_window.visible = false
    @target_window.active = false
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
    @help_window.dispose
    @status_window.dispose
    @skill_window.dispose
    @target_window.dispose
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    # Update windows
    @help_window.update
    @status_window.update
    @skill_window.update
    @target_window.update
    # If skill window is active: call update_skill
    if @skill_window.active
      update_skill
      return
    end
    # If skill target is active: call update_target
    if @target_window.active
      update_target
      return
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (if skill window is active)
  #--------------------------------------------------------------------------
  def update_skill
    # If B button was pressed
    if Input.trigger?(Input::CANCEL)
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # Switch to menu screen
      $scene = Scene_Menu.new(1)
      return
    end
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # Get currently selected data on the skill window
      @skill = @skill_window.skill
      # If unable to use
      if @skill == nil or not @actor.skill_can_use?(@skill.id)
        # Play buzzer SE
        $game_system.se_play($data_system.buzzer_se)
        return
      end
      # Play decision SE
      $game_system.se_play($data_system.decision_se)
      # If effect scope is ally
      if @skill.scope >= 3
        # Activate target window
        @skill_window.active = false
        @target_window.x = (@skill_window.index + 1) % 2 * 304
        @target_window.visible = true
        @target_window.active = true
        # Set cursor position to effect scope (single / all)
        if @skill.scope == 4 || @skill.scope == 6
          @target_window.index = -1
        elsif @skill.scope == 7
          @target_window.index = @actor_index - 10
        else
          @target_window.index = 0
        end
      # If effect scope is other than ally
      else
        # If common event ID is valid
        if @skill.common_event_id > 0
          # Common event call reservation
          $game_temp.common_event_id = @skill.common_event_id
          # Play use skill SE
          $game_system.se_play(@skill.menu_se)
          # Use up SP
          @actor.sp -= @skill.sp_cost
          # Remake each window content
          @status_window.refresh
          @skill_window.refresh
          @target_window.refresh
          # Switch to map screen
          $scene = Scene_Map.new
          return
        end
      end
      return
    end
    # If R button was pressed
    if Input.trigger?(Input::R)
      # Play cursor SE
      $game_system.se_play($data_system.cursor_se)
      # To next actor
      @actor_index += 1
      @actor_index %= $game_party.actors.size
      # Switch to different skill screen
      $scene = Scene_Skill.new(@actor_index)
      return
    end
    # If L button was pressed
    if Input.trigger?(Input::L)
      # Play cursor SE
      $game_system.se_play($data_system.cursor_se)
      # To previous actor
      @actor_index += $game_party.actors.size - 1
      @actor_index %= $game_party.actors.size
      # Switch to different skill screen
      $scene = Scene_Skill.new(@actor_index)
      return
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (when target window is active)
  #--------------------------------------------------------------------------
  def update_target
    # If B button was pressed
    if Input.trigger?(Input::CANCEL)
      # Play cancel SE
      $game_system.se_play($data_system.cancel_se)
      # Erase target window
      @skill_window.active = true
      @target_window.visible = false
      @target_window.active = false
      return
    end
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # If unable to use because SP ran out
      unless @actor.skill_can_use?(@skill.id)
        # Play buzzer SE
        $game_system.se_play($data_system.buzzer_se)
        return
      end
      # If target is all
      if @target_window.index == -1
        # Apply skill use effects to entire party
        used = false
        for i in $game_party.actors
          used |= i.skill_effect(@actor, @skill)
        end
      end
      # If target is user
      if @target_window.index <= -2
        # Apply skill use effects to target actor
        target = $game_party.actors[@target_window.index + 10]
        used = target.skill_effect(@actor, @skill)
      end
      # If single target
      if @target_window.index >= 0
        # Apply skill use effects to target actor
        target = $game_party.actors[@target_window.index]
        used = target.skill_effect(@actor, @skill)
      end
      # If skill was used
      if used
        # Play skill use SE
        $game_system.se_play(@skill.menu_se)
        # Use up SP
        @actor.sp -= @skill.sp_cost
        # Remake each window content
        @status_window.refresh
        @skill_window.refresh
        @target_window.refresh
        # If entire party is dead
        if $game_party.all_dead?
          # Switch to game over screen
          $scene = Scene_Gameover.new
          return
        end
        # If command event ID is valid
        if @skill.common_event_id > 0
          # Command event call reservation
          $game_temp.common_event_id = @skill.common_event_id
          # Switch to map screen
          $scene = Scene_Map.new
          return
        end
      end
      # If skill wasn't used
      unless used
        # Play buzzer SE
        $game_system.se_play($data_system.buzzer_se)
      end
      return
    end
  end
end
