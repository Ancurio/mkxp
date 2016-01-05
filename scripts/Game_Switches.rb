#==============================================================================
# ** Game_Switches
#------------------------------------------------------------------------------
#  This class handles switches. It's a wrapper for the built-in class "Array."
#  Refer to "$game_switches" for the instance of this class.
#==============================================================================

class Game_Switches
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    @data = []
  end
  #--------------------------------------------------------------------------
  # * Get Switch
  #     switch_id : switch ID
  #--------------------------------------------------------------------------
  def [](switch_id)
    if switch_id <= 5000 and @data[switch_id] != nil
      return @data[switch_id]
    else
      return false
    end
  end
  #--------------------------------------------------------------------------
  # * Set Switch
  #     switch_id : switch ID
  #     value     : ON (true) / OFF (false)
  #--------------------------------------------------------------------------
  def []=(switch_id, value)
    if switch_id <= 5000
      @data[switch_id] = value
    end
  end
end
