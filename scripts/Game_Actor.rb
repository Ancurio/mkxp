#==============================================================================
# ** Game_Actor
#------------------------------------------------------------------------------
#  This class handles the actor. It's used within the Game_Actors class
#  ($game_actors) and refers to the Game_Party class ($game_party).
#==============================================================================

class Game_Actor < Game_Battler
  #--------------------------------------------------------------------------
  # * Public Instance Variables
  #--------------------------------------------------------------------------
  attr_reader   :name                     # name
  attr_reader   :character_name           # character file name
  attr_reader   :character_hue            # character hue
  attr_reader   :armor1_id                # shield ID
  attr_reader   :armor2_id                # helmet ID
  attr_reader   :armor3_id                # body armor ID
  attr_reader   :armor4_id                # accessory ID
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     actor_id : actor ID
  #--------------------------------------------------------------------------
  def initialize(actor_id)
    super()
    setup(actor_id)
  end
  #--------------------------------------------------------------------------
  # * Setup
  #     actor_id : actor ID
  #--------------------------------------------------------------------------
  def setup(actor_id)
    actor = $data_actors[actor_id]
    @actor_id = actor_id
    @name = actor.name
    @character_name = actor.character_name
    @character_hue = actor.character_hue
    @class_id = actor.class_id
    @armor1_id = actor.armor1_id
    @armor2_id = actor.armor2_id
    @armor3_id = actor.armor3_id
    @armor4_id = actor.armor4_id
  end
  #--------------------------------------------------------------------------
  # * Get Actor ID
  #--------------------------------------------------------------------------
  def id
    return @actor_id
  end
  #--------------------------------------------------------------------------
  # * Get Index
  #--------------------------------------------------------------------------
  def index
    return $game_party.actors.index(self)
  end
  #--------------------------------------------------------------------------
  # * Determine Fixed Equipment
  #     equip_type : type of equipment
  #--------------------------------------------------------------------------
  def equip_fix?(equip_type)
    case equip_type
    when 0  # Weapon
      return $data_actors[@actor_id].weapon_fix
    when 1  # Shield
      return $data_actors[@actor_id].armor1_fix
    when 2  # Head
      return $data_actors[@actor_id].armor2_fix
    when 3  # Body
      return $data_actors[@actor_id].armor3_fix
    when 4  # Accessory
      return $data_actors[@actor_id].armor4_fix
    end
    return false
  end
  #--------------------------------------------------------------------------
  # * Change Equipment
  #     equip_type : type of equipment
  #     id    : weapon or armor ID (If 0, remove equipment)
  #--------------------------------------------------------------------------
  def equip(equip_type, id)
    case equip_type
    when 0  # Weapon
      if id == 0 or $game_party.weapon_number(id) > 0
        $game_party.gain_weapon(@weapon_id, 1)
        @weapon_id = id
        $game_party.lose_weapon(id, 1)
      end
    when 1  # Shield
      if id == 0 or $game_party.armor_number(id) > 0
        update_auto_state($data_armors[@armor1_id], $data_armors[id])
        $game_party.gain_armor(@armor1_id, 1)
        @armor1_id = id
        $game_party.lose_armor(id, 1)
      end
    when 2  # Head
      if id == 0 or $game_party.armor_number(id) > 0
        update_auto_state($data_armors[@armor2_id], $data_armors[id])
        $game_party.gain_armor(@armor2_id, 1)
        @armor2_id = id
        $game_party.lose_armor(id, 1)
      end
    when 3  # Body
      if id == 0 or $game_party.armor_number(id) > 0
        update_auto_state($data_armors[@armor3_id], $data_armors[id])
        $game_party.gain_armor(@armor3_id, 1)
        @armor3_id = id
        $game_party.lose_armor(id, 1)
      end
    when 4  # Accessory
      if id == 0 or $game_party.armor_number(id) > 0
        update_auto_state($data_armors[@armor4_id], $data_armors[id])
        $game_party.gain_armor(@armor4_id, 1)
        @armor4_id = id
        $game_party.lose_armor(id, 1)
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Determine if Equippable
  #     item : item
  #--------------------------------------------------------------------------
  def equippable?(item)
    # If weapon
    if item.is_a?(RPG::Weapon)
      # If included among equippable weapons in current class
      if $data_classes[@class_id].weapon_set.include?(item.id)
        return true
      end
    end
    # If armor
    if item.is_a?(RPG::Armor)
      # If included among equippable armor in current class
      if $data_classes[@class_id].armor_set.include?(item.id)
        return true
      end
    end
    return false
  end
  #--------------------------------------------------------------------------
  # * Change Name
  #     name : new name
  #--------------------------------------------------------------------------
  def name=(name)
    @name = name
  end
  #--------------------------------------------------------------------------
  # * Change Graphics
  #     character_name : new character file name
  #     character_hue  : new character hue
  #     battler_name   : new battler file name
  #     battler_hue    : new battler hue
  #--------------------------------------------------------------------------
  def set_graphic(character_name, character_hue, battler_name, battler_hue)
    @character_name = character_name
    @character_hue = character_hue
  end
end
