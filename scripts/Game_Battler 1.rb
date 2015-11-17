#==============================================================================
# ** Game_Battler (part 1)
#------------------------------------------------------------------------------
#  This class deals with battlers. It's used as a superclass for the Game_Actor
#  and Game_Enemy classes.
#==============================================================================

class Game_Battler
  #--------------------------------------------------------------------------
  # * Public Instance Variables
  #--------------------------------------------------------------------------
  attr_reader   :battler_name             # battler file name
  attr_reader   :battler_hue              # battler hue
  attr_reader   :hp                       # HP
  attr_reader   :sp                       # SP
  attr_reader   :states                   # states
  attr_accessor :hidden                   # hidden flag
  attr_accessor :immortal                 # immortal flag
  attr_accessor :damage_pop               # damage display flag
  attr_accessor :damage                   # damage value
  attr_accessor :critical                 # critical flag
  attr_accessor :animation_id             # animation ID
  attr_accessor :animation_hit            # animation hit flag
  attr_accessor :white_flash              # white flash flag
  attr_accessor :blink                    # blink flag
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    @battler_name = ""
    @battler_hue = 0
    @hp = 0
    @sp = 0
    @states = []
    @states_turn = {}
    @maxhp_plus = 0
    @maxsp_plus = 0
    @str_plus = 0
    @dex_plus = 0
    @agi_plus = 0
    @int_plus = 0
    @hidden = false
    @immortal = false
    @damage_pop = false
    @damage = nil
    @critical = false
    @animation_id = 0
    @animation_hit = false
    @white_flash = false
    @blink = false
    @current_action = Game_BattleAction.new
  end
  #--------------------------------------------------------------------------
  # * Get Maximum HP
  #--------------------------------------------------------------------------
  def maxhp
    n = [[base_maxhp + @maxhp_plus, 1].max, 999999].min
    for i in @states
      n *= $data_states[i].maxhp_rate / 100.0
    end
    n = [[Integer(n), 1].max, 999999].min
    return n
  end
  #--------------------------------------------------------------------------
  # * Get Maximum SP
  #--------------------------------------------------------------------------
  def maxsp
    n = [[base_maxsp + @maxsp_plus, 0].max, 9999].min
    for i in @states
      n *= $data_states[i].maxsp_rate / 100.0
    end
    n = [[Integer(n), 0].max, 9999].min
    return n
  end
  #--------------------------------------------------------------------------
  # * Get Strength (STR)
  #--------------------------------------------------------------------------
  def str
    n = [[base_str + @str_plus, 1].max, 999].min
    for i in @states
      n *= $data_states[i].str_rate / 100.0
    end
    n = [[Integer(n), 1].max, 999].min
    return n
  end
  #--------------------------------------------------------------------------
  # * Get Dexterity (DEX)
  #--------------------------------------------------------------------------
  def dex
    n = [[base_dex + @dex_plus, 1].max, 999].min
    for i in @states
      n *= $data_states[i].dex_rate / 100.0
    end
    n = [[Integer(n), 1].max, 999].min
    return n
  end
  #--------------------------------------------------------------------------
  # * Get Agility (AGI)
  #--------------------------------------------------------------------------
  def agi
    n = [[base_agi + @agi_plus, 1].max, 999].min
    for i in @states
      n *= $data_states[i].agi_rate / 100.0
    end
    n = [[Integer(n), 1].max, 999].min
    return n
  end
  #--------------------------------------------------------------------------
  # * Get Intelligence (INT)
  #--------------------------------------------------------------------------
  def int
    n = [[base_int + @int_plus, 1].max, 999].min
    for i in @states
      n *= $data_states[i].int_rate / 100.0
    end
    n = [[Integer(n), 1].max, 999].min
    return n
  end
  #--------------------------------------------------------------------------
  # * Set Maximum HP
  #     maxhp : new maximum HP
  #--------------------------------------------------------------------------
  def maxhp=(maxhp)
    @maxhp_plus += maxhp - self.maxhp
    @maxhp_plus = [[@maxhp_plus, -9999].max, 9999].min
    @hp = [@hp, self.maxhp].min
  end
  #--------------------------------------------------------------------------
  # * Set Maximum SP
  #     maxsp : new maximum SP
  #--------------------------------------------------------------------------
  def maxsp=(maxsp)
    @maxsp_plus += maxsp - self.maxsp
    @maxsp_plus = [[@maxsp_plus, -9999].max, 9999].min
    @sp = [@sp, self.maxsp].min
  end
  #--------------------------------------------------------------------------
  # * Set Strength (STR)
  #     str : new Strength (STR)
  #--------------------------------------------------------------------------
  def str=(str)
    @str_plus += str - self.str
    @str_plus = [[@str_plus, -999].max, 999].min
  end
  #--------------------------------------------------------------------------
  # * Set Dexterity (DEX)
  #     dex : new Dexterity (DEX)
  #--------------------------------------------------------------------------
  def dex=(dex)
    @dex_plus += dex - self.dex
    @dex_plus = [[@dex_plus, -999].max, 999].min
  end
  #--------------------------------------------------------------------------
  # * Set Agility (AGI)
  #     agi : new Agility (AGI)
  #--------------------------------------------------------------------------
  def agi=(agi)
    @agi_plus += agi - self.agi
    @agi_plus = [[@agi_plus, -999].max, 999].min
  end
  #--------------------------------------------------------------------------
  # * Set Intelligence (INT)
  #     int : new Intelligence (INT)
  #--------------------------------------------------------------------------
  def int=(int)
    @int_plus += int - self.int
    @int_plus = [[@int_plus, -999].max, 999].min
  end
  #--------------------------------------------------------------------------
  # * Get Hit Rate
  #--------------------------------------------------------------------------
  def hit
    n = 100
    for i in @states
      n *= $data_states[i].hit_rate / 100.0
    end
    return Integer(n)
  end
  #--------------------------------------------------------------------------
  # * Get Attack Power
  #--------------------------------------------------------------------------
  def atk
    n = base_atk
    for i in @states
      n *= $data_states[i].atk_rate / 100.0
    end
    return Integer(n)
  end
  #--------------------------------------------------------------------------
  # * Get Physical Defense Power
  #--------------------------------------------------------------------------
  def pdef
    n = base_pdef
    for i in @states
      n *= $data_states[i].pdef_rate / 100.0
    end
    return Integer(n)
  end
  #--------------------------------------------------------------------------
  # * Get Magic Defense Power
  #--------------------------------------------------------------------------
  def mdef
    n = base_mdef
    for i in @states
      n *= $data_states[i].mdef_rate / 100.0
    end
    return Integer(n)
  end
  #--------------------------------------------------------------------------
  # * Get Evasion Correction
  #--------------------------------------------------------------------------
  def eva
    n = base_eva
    for i in @states
      n += $data_states[i].eva
    end
    return n
  end
  #--------------------------------------------------------------------------
  # * Change HP
  #     hp : new HP
  #--------------------------------------------------------------------------
  def hp=(hp)
    @hp = [[hp, maxhp].min, 0].max
    # add or exclude incapacitation
    for i in 1...$data_states.size
      if $data_states[i].zero_hp
        if self.dead?
          add_state(i)
        else
          remove_state(i)
        end
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Change SP
  #     sp : new SP
  #--------------------------------------------------------------------------
  def sp=(sp)
    @sp = [[sp, maxsp].min, 0].max
  end
  #--------------------------------------------------------------------------
  # * Recover All
  #--------------------------------------------------------------------------
  def recover_all
    @hp = maxhp
    @sp = maxsp
    for i in @states.clone
      remove_state(i)
    end
  end
  #--------------------------------------------------------------------------
  # * Get Current Action
  #--------------------------------------------------------------------------
  def current_action
    return @current_action
  end
  #--------------------------------------------------------------------------
  # * Determine Action Speed
  #--------------------------------------------------------------------------
  def make_action_speed
    @current_action.speed = agi + rand(10 + agi / 4)
  end
  #--------------------------------------------------------------------------
  # * Decide Incapacitation
  #--------------------------------------------------------------------------
  def dead?
    return (@hp == 0 and not @immortal)
  end
  #--------------------------------------------------------------------------
  # * Decide Existance
  #--------------------------------------------------------------------------
  def exist?
    return (not @hidden and (@hp > 0 or @immortal))
  end
  #--------------------------------------------------------------------------
  # * Decide HP 0
  #--------------------------------------------------------------------------
  def hp0?
    return (not @hidden and @hp == 0)
  end
  #--------------------------------------------------------------------------
  # * Decide if Command is Inputable
  #--------------------------------------------------------------------------
  def inputable?
    return (not @hidden and restriction <= 1)
  end
  #--------------------------------------------------------------------------
  # * Decide if Action is Possible
  #--------------------------------------------------------------------------
  def movable?
    return (not @hidden and restriction < 4)
  end
  #--------------------------------------------------------------------------
  # * Decide if Guarding
  #--------------------------------------------------------------------------
  def guarding?
    return (@current_action.kind == 0 and @current_action.basic == 1)
  end
  #--------------------------------------------------------------------------
  # * Decide if Resting
  #--------------------------------------------------------------------------
  def resting?
    return (@current_action.kind == 0 and @current_action.basic == 3)
  end
end
