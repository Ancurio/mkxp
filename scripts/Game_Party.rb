#==============================================================================
# ** Game_Party
#------------------------------------------------------------------------------
#  This class handles the party. It includes information on amount of gold
#  and items. Refer to "$game_party" for the instance of this class.
#==============================================================================

class Game_Party
  #--------------------------------------------------------------------------
  # * Public Instance Variables
  #--------------------------------------------------------------------------
  attr_reader   :actors                   # actors
  attr_reader   :gold                     # amount of gold
  attr_reader   :steps                    # number of steps
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    # Create actor array
    @actors = []
    # Initialize amount of gold and steps
    @gold = 0
    @steps = 0
    # Create amount in possession hash for items, weapons, and armor
    @items = {}
    @weapons = {}
    @armors = {}
  end
  #--------------------------------------------------------------------------
  # * Initial Party Setup
  #--------------------------------------------------------------------------
  def setup_starting_members
    @actors = []
    for i in $data_system.party_members
      #@actors.push($game_actors[i])
      add_actor(i)
    end
  end
  #--------------------------------------------------------------------------
  # * Battle Test Party Setup
  #--------------------------------------------------------------------------
  def setup_battle_test_members
    @actors = []
    for battler in $data_system.test_battlers
      actor = $game_actors[battler.actor_id]
      actor.level = battler.level
      gain_weapon(battler.weapon_id, 1)
      gain_armor(battler.armor1_id, 1)
      gain_armor(battler.armor2_id, 1)
      gain_armor(battler.armor3_id, 1)
      gain_armor(battler.armor4_id, 1)
      actor.equip(0, battler.weapon_id)
      actor.equip(1, battler.armor1_id)
      actor.equip(2, battler.armor2_id)
      actor.equip(3, battler.armor3_id)
      actor.equip(4, battler.armor4_id)
      actor.recover_all
      @actors.push(actor)
    end
    @items = {}
    for i in 1...$data_items.size
      if $data_items[i].name != ""
        occasion = $data_items[i].occasion
        if occasion == 0 or occasion == 1
          @items[i] = 99
        end
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Refresh Party Members
  #--------------------------------------------------------------------------
  def refresh
    # Actor objects split from $game_actors right after loading game data
    # Avoid this problem by resetting the actors each time data is loaded.
    new_actors = []
    for i in 0...@actors.size
      if $data_actors[@actors[i].id] != nil
        new_actors.push($game_actors[@actors[i].id])
      end
    end
    @actors = new_actors
  end
  #--------------------------------------------------------------------------
  # * Getting Maximum Level
  #--------------------------------------------------------------------------
  def max_level
    # If 0 members are in the party
    if @actors.size == 0
      return 0
    end
    # Initialize local variable: level
    level = 0
    # Get maximum level of party members
    for actor in @actors
      if level < actor.level
        level = actor.level
      end
    end
    return level
  end
  #--------------------------------------------------------------------------
  # * Add an Actor
  #     actor_id : actor ID
  #--------------------------------------------------------------------------
  def add_actor(actor_id)
    # Get actor
    actor = $game_actors[actor_id]
    # If the party has less than 4 members and this actor is not in the party
    if @actors.size < 4 and not @actors.include?(actor)
      # Add follower
      unless @actors.empty?
        if $game_followers.empty?
          $game_followers.push(Game_Follower.new($game_player, actor))
        else
          $game_followers.push(Game_Follower.new($game_followers.last, actor))
        end
        $scene.add_follower($game_followers.last)
      end
      # Add actor
      @actors.push(actor)
      # Refresh player
      $game_player.refresh
    end
  end
  #--------------------------------------------------------------------------
  # * Remove Actor
  #     actor_id : actor ID
  #--------------------------------------------------------------------------
  def remove_actor(actor_id)
    actor = $game_actors[actor_id]
    # Delete follower
    unless $game_followers.empty?
      follower = $game_followers.pop
      $scene.remove_follower(follower)
      new_actor = follower.actor
      $game_followers.reverse_each do |follower|
        new_actor, follower.actor = follower.actor, new_actor
        break if new_actor == actor
      end
    end
    # Delete actor
    @actors.delete(actor)
    # Refresh player
    $game_player.refresh
  end
  #--------------------------------------------------------------------------
  # * Gain Gold (or lose)
  #     n : amount of gold
  #--------------------------------------------------------------------------
  def gain_gold(n)
    @gold = [[@gold + n, 0].max, 9999999].min
  end
  #--------------------------------------------------------------------------
  # * Lose Gold
  #     n : amount of gold
  #--------------------------------------------------------------------------
  def lose_gold(n)
    # Reverse the numerical value and call it gain_gold
    gain_gold(-n)
  end
  #--------------------------------------------------------------------------
  # * Increase Steps
  #--------------------------------------------------------------------------
  def increase_steps
    @steps = [@steps + 1, 9999999].min
  end
  #--------------------------------------------------------------------------
  # * Get Number of Items Possessed
  #     item_id : item ID
  #--------------------------------------------------------------------------
  def item_number(item_id)
    # If quantity data is in the hash, use it. If not, return 0
    return @items.include?(item_id) ? @items[item_id] : 0
  end
  #--------------------------------------------------------------------------
  # * Get Number of Weapons Possessed
  #     weapon_id : weapon ID
  #--------------------------------------------------------------------------
  def weapon_number(weapon_id)
    # If quantity data is in the hash, use it. If not, return 0
    return @weapons.include?(weapon_id) ? @weapons[weapon_id] : 0
  end
  #--------------------------------------------------------------------------
  # * Get Amount of Armor Possessed
  #     armor_id : armor ID
  #--------------------------------------------------------------------------
  def armor_number(armor_id)
    # If quantity data is in the hash, use it. If not, return 0
    return @armors.include?(armor_id) ? @armors[armor_id] : 0
  end
  #--------------------------------------------------------------------------
  # * Gain Items (or lose)
  #     item_id : item ID
  #     n       : quantity
  #--------------------------------------------------------------------------
  def gain_item(item_id, n)
    # Update quantity data in the hash.
    if item_id > 0
      @items[item_id] = [[item_number(item_id) + n, 0].max, 99].min
    end
  end
  #--------------------------------------------------------------------------
  # * Gain Weapons (or lose)
  #     weapon_id : weapon ID
  #     n         : quantity
  #--------------------------------------------------------------------------
  def gain_weapon(weapon_id, n)
    # Update quantity data in the hash.
    if weapon_id > 0
      @weapons[weapon_id] = [[weapon_number(weapon_id) + n, 0].max, 99].min
    end
  end
  #--------------------------------------------------------------------------
  # * Gain Armor (or lose)
  #     armor_id : armor ID
  #     n        : quantity
  #--------------------------------------------------------------------------
  def gain_armor(armor_id, n)
    # Update quantity data in the hash.
    if armor_id > 0
      @armors[armor_id] = [[armor_number(armor_id) + n, 0].max, 99].min
    end
  end
  #--------------------------------------------------------------------------
  # * Lose Items
  #     item_id : item ID
  #     n       : quantity
  #--------------------------------------------------------------------------
  def lose_item(item_id, n)
    # Reverse the numerical value and call it gain_item
    gain_item(item_id, -n)
  end
  #--------------------------------------------------------------------------
  # * Lose Weapons
  #     weapon_id : weapon ID
  #     n         : quantity
  #--------------------------------------------------------------------------
  def lose_weapon(weapon_id, n)
    # Reverse the numerical value and call it gain_weapon
    gain_weapon(weapon_id, -n)
  end
  #--------------------------------------------------------------------------
  # * Lose Armor
  #     armor_id : armor ID
  #     n        : quantity
  #--------------------------------------------------------------------------
  def lose_armor(armor_id, n)
    # Reverse the numerical value and call it gain_armor
    gain_armor(armor_id, -n)
  end
  #--------------------------------------------------------------------------
  # * Determine if Item is Usable
  #     item_id : item ID
  #--------------------------------------------------------------------------
  def item_can_use?(item_id)
    # If item quantity is 0
    if item_number(item_id) == 0
      # Unusable
      return false
    end
    # Get usable time
    occasion = $data_items[item_id].occasion
    # If in battle
    if $game_temp.in_battle
      # If useable time is 0 (normal) or 1 (only battle) it's usable
      return (occasion == 0 or occasion == 1)
    end
    # If useable time is 0 (normal) or 2 (only menu) it's usable
    return (occasion == 0 or occasion == 2)
  end
  #--------------------------------------------------------------------------
  # * Clear All Member Actions
  #--------------------------------------------------------------------------
  def clear_actions
    # Clear All Member Actions
    for actor in @actors
      actor.current_action.clear
    end
  end
  #--------------------------------------------------------------------------
  # * Determine if Command is Inputable
  #--------------------------------------------------------------------------
  def inputable?
    # Return true if input is possible for one person as well
    for actor in @actors
      if actor.inputable?
        return true
      end
    end
    return false
  end
  #--------------------------------------------------------------------------
  # * Determine Everyone is Dead
  #--------------------------------------------------------------------------
  def all_dead?
    # If number of party members is 0
    if $game_party.actors.size == 0
      return false
    end
    # If an actor is in the party with 0 or more HP
    for actor in @actors
      if actor.hp > 0
        return false
      end
    end
    # All members dead
    return true
  end
  #--------------------------------------------------------------------------
  # * Slip Damage Check (for map)
  #--------------------------------------------------------------------------
  def check_map_slip_damage
    for actor in @actors
      if actor.hp > 0 and actor.slip_damage?
        actor.hp -= [actor.maxhp / 100, 1].max
        if actor.hp == 0
          $game_system.se_play($data_system.actor_collapse_se)
        end
        $game_screen.start_flash(Color.new(255,0,0,128), 4)
        $game_temp.gameover = $game_party.all_dead?
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Random Selection of Target Actor
  #     hp0 : limited to actors with 0 HP
  #--------------------------------------------------------------------------
  def random_target_actor(hp0 = false)
    # Initialize roulette
    roulette = []
    # Loop
    for actor in @actors
      # If it fits the conditions
      if (not hp0 and actor.exist?) or (hp0 and actor.hp0?)
        # Get actor class [position]
        position = $data_classes[actor.class_id].position
        # Front guard: n = 4; Mid guard: n = 3; Rear guard: n = 2
        n = 4 - position
        # Add actor to roulette n times
        n.times do
          roulette.push(actor)
        end
      end
    end
    # If roulette size is 0
    if roulette.size == 0
      return nil
    end
    # Spin the roulette, choose an actor
    return roulette[rand(roulette.size)]
  end
  #--------------------------------------------------------------------------
  # * Random Selection of Target Actor (HP 0)
  #--------------------------------------------------------------------------
  def random_target_actor_hp0
    return random_target_actor(true)
  end
  #--------------------------------------------------------------------------
  # * Smooth Selection of Target Actor
  #     actor_index : actor index
  #--------------------------------------------------------------------------
  def smooth_target_actor(actor_index)
    # Get an actor
    actor = @actors[actor_index]
    # If an actor exists
    if actor != nil and actor.exist?
      return actor
    end
    # Loop
    for actor in @actors
      # If an actor exists
      if actor.exist?
        return actor
      end
    end
  end
end
