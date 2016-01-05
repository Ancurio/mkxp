#==============================================================================
# ** Game_Picture
#------------------------------------------------------------------------------
#  This class handles the picture. It's used within the Game_Screen class
#  ($game_screen).
#==============================================================================

class Game_Picture
  #--------------------------------------------------------------------------
  # * Public Instance Variables
  #--------------------------------------------------------------------------
  attr_reader   :number                   # picture number
  attr_reader   :name                     # file name
  attr_reader   :origin                   # starting point
  attr_reader   :x                        # x-coordinate
  attr_reader   :y                        # y-coordinate
  attr_reader   :zoom_x                   # x directional zoom rate
  attr_reader   :zoom_y                   # y directional zoom rate
  attr_reader   :opacity                  # opacity level
  attr_reader   :blend_type               # blend method
  attr_reader   :tone                     # color tone
  attr_reader   :angle                    # rotation angle
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     number : picture number
  #--------------------------------------------------------------------------
  def initialize(number)
    @number = number
    @name = ""
    @origin = 0
    @x = 0.0
    @y = 0.0
    @zoom_x = 100.0
    @zoom_y = 100.0
    @opacity = 255.0
    @blend_type = 1
    @duration = 0
    @target_x = @x
    @target_y = @y
    @target_zoom_x = @zoom_x
    @target_zoom_y = @zoom_y
    @target_opacity = @opacity
    @tone = Tone.new(0, 0, 0, 0)
    @tone_target = Tone.new(0, 0, 0, 0)
    @tone_duration = 0
    @angle = 0
    @rotate_speed = 0
  end
  #--------------------------------------------------------------------------
  # * Show Picture
  #     name       : file name
  #     origin     : starting point
  #     x          : x-coordinate
  #     y          : y-coordinate
  #     zoom_x     : x directional zoom rate
  #     zoom_y     : y directional zoom rate
  #     opacity    : opacity level
  #     blend_type : blend method
  #--------------------------------------------------------------------------
  def show(name, origin, x, y, zoom_x, zoom_y, opacity, blend_type)
    @name = name
    @origin = origin
    @x = x.to_f
    @y = y.to_f
    @zoom_x = zoom_x.to_f
    @zoom_y = zoom_y.to_f
    @opacity = opacity.to_f
    @blend_type = blend_type
    @duration = 0
    @target_x = @x
    @target_y = @y
    @target_zoom_x = @zoom_x
    @target_zoom_y = @zoom_y
    @target_opacity = @opacity
    @tone = Tone.new(0, 0, 0, 0)
    @tone_target = Tone.new(0, 0, 0, 0)
    @tone_duration = 0
    @angle = 0
    @rotate_speed = 0
  end
  #--------------------------------------------------------------------------
  # * Move Picture
  #     duration   : time
  #     origin     : starting point
  #     x          : x-coordinate
  #     y          : y-coordinate
  #     zoom_x     : x directional zoom rate
  #     zoom_y     : y directional zoom rate
  #     opacity    : opacity level
  #     blend_type : blend method
  #--------------------------------------------------------------------------
  def move(duration, origin, x, y, zoom_x, zoom_y, opacity, blend_type)
    @duration = duration
    @origin = origin
    @target_x = x.to_f
    @target_y = y.to_f
    @target_zoom_x = zoom_x.to_f
    @target_zoom_y = zoom_y.to_f
    @target_opacity = opacity.to_f
    @blend_type = blend_type
  end
  #--------------------------------------------------------------------------
  # * Change Rotation Speed
  #     speed : rotation speed
  #--------------------------------------------------------------------------
  def rotate(speed)
    @rotate_speed = speed
  end
  #--------------------------------------------------------------------------
  # * Start Change of Color Tone
  #     tone     : color tone
  #     duration : time
  #--------------------------------------------------------------------------
  def start_tone_change(tone, duration)
    @tone_target = tone.clone
    @tone_duration = duration
    if @tone_duration == 0
      @tone = @tone_target.clone
    end
  end
  #--------------------------------------------------------------------------
  # * Erase Picture
  #--------------------------------------------------------------------------
  def erase
    @name = ""
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    if @duration >= 1
      d = @duration
      @x = (@x * (d - 1) + @target_x) / d
      @y = (@y * (d - 1) + @target_y) / d
      @zoom_x = (@zoom_x * (d - 1) + @target_zoom_x) / d
      @zoom_y = (@zoom_y * (d - 1) + @target_zoom_y) / d
      @opacity = (@opacity * (d - 1) + @target_opacity) / d
      @duration -= 1
    end
    if @tone_duration >= 1
      d = @tone_duration
      @tone.red = (@tone.red * (d - 1) + @tone_target.red) / d
      @tone.green = (@tone.green * (d - 1) + @tone_target.green) / d
      @tone.blue = (@tone.blue * (d - 1) + @tone_target.blue) / d
      @tone.gray = (@tone.gray * (d - 1) + @tone_target.gray) / d
      @tone_duration -= 1
    end
    if @rotate_speed != 0
      @angle += @rotate_speed / 2.0
      while @angle < 0
        @angle += 360
      end
      @angle %= 360
    end
  end
end
