#==============================================================================
# ** Sprite_Timer
#------------------------------------------------------------------------------
#  This sprite is used to display the timer.It observes the $game_system
#  class and automatically changes sprite conditions.
#==============================================================================

class Sprite_Timer < Sprite
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    super
    self.bitmap = Bitmap.new(88, 48)
    self.bitmap.font.name = "Arial"
    self.bitmap.font.size = 32
    self.x = 640 - self.bitmap.width
    self.y = 0
    self.z = 500
    update
  end
  #--------------------------------------------------------------------------
  # * Dispose
  #--------------------------------------------------------------------------
  def dispose
    if self.bitmap != nil
      self.bitmap.dispose
    end
    super
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    super
    # Set timer to visible if working
    self.visible = $game_system.timer_working
    # If timer needs to be redrawn
    if $game_system.timer / Graphics.frame_rate != @total_sec
      # Clear window contents
      self.bitmap.clear
      # Calculate total number of seconds
      @total_sec = $game_system.timer / Graphics.frame_rate
      # Make a string for displaying the timer
      min = @total_sec / 60
      sec = @total_sec % 60
      text = sprintf("%02d:%02d", min, sec)
      # Draw timer
      self.bitmap.font.color.set(255, 255, 255)
      self.bitmap.draw_text(self.bitmap.rect, text, 1)
    end
  end
end
