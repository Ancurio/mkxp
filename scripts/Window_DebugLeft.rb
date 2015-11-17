#==============================================================================
# ** Window_DebugLeft
#------------------------------------------------------------------------------
#  This window designates switch and variable blocks on the debug screen.
#==============================================================================

class Window_DebugLeft < Window_Selectable
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    super(0, 0, 192, 480)
    self.index = 0
    refresh
  end
  #--------------------------------------------------------------------------
  # * Refresh
  #--------------------------------------------------------------------------
  def refresh
    if self.contents != nil
      self.contents.dispose
      self.contents = nil
    end
    @switch_max = ($data_system.switches.size - 1 + 9) / 10
    @variable_max = ($data_system.variables.size - 1 + 9) / 10
    @item_max = @switch_max + @variable_max
    self.contents = Bitmap.new(width - 32, @item_max * 32)
    for i in 0...@switch_max
      text = sprintf("S [%04d-%04d]", i*10+1, i*10+10)
      self.contents.draw_text(4, i * 32, 152, 32, text)
    end
    for i in 0...@variable_max
      text = sprintf("V [%04d-%04d]", i*10+1, i*10+10)
      self.contents.draw_text(4, (@switch_max + i) * 32, 152, 32, text)
    end
  end
  #--------------------------------------------------------------------------
  # * Get Mode
  #--------------------------------------------------------------------------
  def mode
    if self.index < @switch_max
      return 0
    else
      return 1
    end
  end
  #--------------------------------------------------------------------------
  # * Get ID Shown on Top
  #--------------------------------------------------------------------------
  def top_id
    if self.index < @switch_max
      return self.index * 10 + 1
    else
      return (self.index - @switch_max) * 10 + 1
    end
  end
end
