#==============================================================================
# ** Game_BattleAction
#------------------------------------------------------------------------------
#  This class handles actions in battle. It's used within the Game_Battler
#  class.
#==============================================================================

class Game_BattleAction
  #--------------------------------------------------------------------------
  # * Public Instance Variables
  #--------------------------------------------------------------------------
  attr_accessor :speed                    # speed
  attr_accessor :kind                     # kind (basic / skill / item)
  attr_accessor :basic                    # basic (attack / guard / escape)
  attr_accessor :skill_id                 # skill ID
  attr_accessor :item_id                  # item ID
  attr_accessor :target_index             # target index
  attr_accessor :forcing                  # forced flag
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    clear
  end
  #--------------------------------------------------------------------------
  # * Clear
  #--------------------------------------------------------------------------
  def clear
    @speed = 0
    @kind = 0
    @basic = 3
    @skill_id = 0
    @item_id = 0
    @target_index = -1
    @forcing = false
  end
  #--------------------------------------------------------------------------
  # * Determine Validity
  #--------------------------------------------------------------------------
  def valid?
    return (not (@kind == 0 and @basic == 3))
  end
  #--------------------------------------------------------------------------
  # * Determine if for One Ally
  #--------------------------------------------------------------------------
  def for_one_friend?
    # If kind = skill, and effect scope is for ally (including 0 HP)
    if @kind == 1 and [3, 5].include?($data_skills[@skill_id].scope)
      return true
    end
    # If kind = item, and effect scope is for ally (including 0 HP)
    if @kind == 2 and [3, 5].include?($data_items[@item_id].scope)
      return true
    end
    return false
  end
  #--------------------------------------------------------------------------
  # * Determine if for One Ally (HP 0)
  #--------------------------------------------------------------------------
  def for_one_friend_hp0?
    # If kind = skill, and effect scope is for ally (only 0 HP)
    if @kind == 1 and [5].include?($data_skills[@skill_id].scope)
      return true
    end
    # If kind = item, and effect scope is for ally (only 0 HP)
    if @kind == 2 and [5].include?($data_items[@item_id].scope)
      return true
    end
    return false
  end
  #--------------------------------------------------------------------------
  # * Random Target (for Actor)
  #--------------------------------------------------------------------------
  def decide_random_target_for_actor
    # Diverge with effect scope
    if for_one_friend_hp0?
      battler = $game_party.random_target_actor_hp0
    elsif for_one_friend?
      battler = $game_party.random_target_actor
    else
      battler = $game_troop.random_target_enemy
    end
    # If a target exists, get an index, and if a target doesn't exist,
    # clear the action
    if battler != nil
      @target_index = battler.index
    else
      clear
    end
  end
  #--------------------------------------------------------------------------
  # * Random Target (for Enemy)
  #--------------------------------------------------------------------------
  def decide_random_target_for_enemy
    # Diverge with effect scope
    if for_one_friend_hp0?
      battler = $game_troop.random_target_enemy_hp0
    elsif for_one_friend?
      battler = $game_troop.random_target_enemy
    else
      battler = $game_party.random_target_actor
    end
    # If a target exists, get an index, and if a target doesn't exist,
    # clear the action
    if battler != nil
      @target_index = battler.index
    else
      clear
    end
  end
  #--------------------------------------------------------------------------
  # * Last Target (for Actor)
  #--------------------------------------------------------------------------
  def decide_last_target_for_actor
    # If effect scope is ally, then it's an actor, anything else is an enemy
    if @target_index == -1
      battler = nil
    elsif for_one_friend?
      battler = $game_party.actors[@target_index]
    else
      battler = $game_troop.enemies[@target_index]
    end
    # Clear action if no target exists
    if battler == nil or not battler.exist?
      clear
    end
  end
  #--------------------------------------------------------------------------
  # * Last Target (for Enemy)
  #--------------------------------------------------------------------------
  def decide_last_target_for_enemy
    # If effect scope is ally, then it's an enemy, anything else is an actor
    if @target_index == -1
      battler = nil
    elsif for_one_friend?
      battler = $game_troop.enemies[@target_index]
    else
      battler = $game_party.actors[@target_index]
    end
    # Clear action if no target exists
    if battler == nil or not battler.exist?
      clear
    end
  end
end
