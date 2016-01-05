#==============================================================================
# ** Game_Variables
#------------------------------------------------------------------------------
#  This class handles variables. It's a wrapper for the built-in class "Array."
#  Refer to "$game_variables" for the instance of this class.
#==============================================================================

class Game_Variables
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    @data = []
  end
  #--------------------------------------------------------------------------
  # * Get Variable
  #     variable_id : variable ID
  #--------------------------------------------------------------------------
  def [](variable_id)
    if variable_id <= 5000 and @data[variable_id] != nil
      return @data[variable_id]
    else
      return 0
    end
  end
  #--------------------------------------------------------------------------
  # * Set Variable
  #     variable_id : variable ID
  #     value       : the variable's value
  #--------------------------------------------------------------------------
  def []=(variable_id, value)
    if variable_id <= 5000
      @data[variable_id] = value
    end
  end
end
