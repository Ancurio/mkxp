#==============================================================================
# ** Game_Character (part 1)
#------------------------------------------------------------------------------
#  This class deals with characters. It's used as a superclass for the
#  Game_Player and Game_Event classes.
#==============================================================================

class Game_Character
  #--------------------------------------------------------------------------
  # * Public Instance Variables
  #--------------------------------------------------------------------------
  attr_reader   :id                       # ID
  attr_reader   :x                        # map x-coordinate (logical)
  attr_reader   :y                        # map y-coordinate (logical)
  attr_reader   :real_x                   # map x-coordinate (real * 128)
  attr_reader   :real_y                   # map y-coordinate (real * 128)
  attr_reader   :tile_id                  # tile ID (invalid if 0)
  attr_reader   :character_name           # character file name
  attr_reader   :character_hue            # character hue
  attr_reader   :opacity                  # opacity level
  attr_reader   :blend_type               # blending method
  attr_reader   :direction                # direction
  attr_reader   :pattern                  # pattern
  attr_reader   :move_route_forcing       # forced move route flag
  attr_reader   :through                  # through
  attr_accessor :animation_id             # animation ID
  attr_accessor :transparent              # transparent flag
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    @id = 0
    @x = 0
    @y = 0
    @real_x = 0
    @real_y = 0
    @tile_id = 0
    @character_name = ""
    @character_hue = 0
    @opacity = 255
    @blend_type = 0
    @direction = 2
    @pattern = 0
    @move_route_forcing = false
    @through = false
    @animation_id = 0
    @transparent = false
    @original_direction = 2
    @original_pattern = 0
    @move_type = 0
    @move_speed = 4
    @move_frequency = 6
    @move_route = nil
    @move_route_index = 0
    @original_move_route = nil
    @original_move_route_index = 0
    @walk_anime = true
    @step_anime = false
    @direction_fix = false
    @always_on_top = false
    @anime_count = 0
    @stop_count = 0
    @jump_count = 0
    @jump_peak = 0
    @wait_count = 0
    @locked = false
    @prelock_direction = 0
    @custom_flags = []
  end
  #--------------------------------------------------------------------------
  # * Determine if Moving
  #--------------------------------------------------------------------------
  def moving?
    # If logical coordinates differ from real coordinates,
    # movement is occurring.
    return (@real_x != @x * 128 or @real_y != @y * 128)
  end
  #--------------------------------------------------------------------------
  # * Determine if Jumping
  #--------------------------------------------------------------------------
  def jumping?
    # A jump is occurring if jump count is larger than 0
    return @jump_count > 0
  end
  #--------------------------------------------------------------------------
  # * Straighten Position
  #--------------------------------------------------------------------------
  def straighten
    # If moving animation or stop animation is ON
    if @walk_anime or @step_anime
      # Set pattern to 0
      @pattern = 0
    end
    # Clear animation count
    @anime_count = 0
    # Clear prelock direction
    @prelock_direction = 0
  end
  #--------------------------------------------------------------------------
  # * Force Move Route
  #     move_route : new move route
  #--------------------------------------------------------------------------
  def force_move_route(move_route)
    # Save original move route
    if @original_move_route == nil
      @original_move_route = @move_route
      @original_move_route_index = @move_route_index
    end
    # Change move route
    @move_route = move_route
    @move_route_index = 0
    # Set forced move route flag
    @move_route_forcing = true
    # Clear prelock direction
    @prelock_direction = 0
    # Clear wait count
    @wait_count = 0
    # Move cutsom
    move_type_custom
  end
  #--------------------------------------------------------------------------
  # * Determine if Passable
  #     x : x-coordinate
  #     y : y-coordinate
  #     d : direction (0,2,4,6,8)
  #         * 0 = Determines if all directions are impassable (for jumping)
  #--------------------------------------------------------------------------
  def passable?(x, y, d)
    # Get new coordinates
    new_x = x + (d == 6 ? 1 : d == 4 ? -1 : 0)
    new_y = y + (d == 2 ? 1 : d == 8 ? -1 : 0)
    # If coordinates are outside of map
    unless $game_map.valid?(new_x, new_y)
      # impassable
      return false
    end
    # If through is ON
    if @through
      # passable
      return true
    end
    # If unable to leave first move tile in designated direction
    unless $game_map.passable?(x, y, d, self)
      # impassable
      return false
    end
    # If unable to enter move tile in designated direction
    unless $game_map.passable?(new_x, new_y, 10 - d)
      # impassable
      return false
    end
    # Loop all events
    for event in $game_map.events.values
      next if event.through || event.character_name.empty?
      event.collision.each do |ox, oy|
        if event.x + ox == new_x && event.y + oy == new_y
          return false
        end
      end
    end
    # If player coordinates are consistent with move destination
    if $game_player.x == new_x and $game_player.y == new_y
      # If through is OFF
      unless $game_player.through
        # If your own graphic is the character
        if @character_name != ""
          # impassable
          return false
        end
      end
    end
    # passable
    return true
  end
  #--------------------------------------------------------------------------
  # * Lock
  #--------------------------------------------------------------------------
  def lock
    # If already locked
    if @locked
      # End method
      return
    end
    # Save prelock direction
    @prelock_direction = @direction
    # Turn toward player
    turn_toward_player
    # Set locked flag
    @locked = true
  end
  #--------------------------------------------------------------------------
  # * Determine if Locked
  #--------------------------------------------------------------------------
  def lock?
    return @locked
  end
  #--------------------------------------------------------------------------
  # * Unlock
  #--------------------------------------------------------------------------
  def unlock
    # If not locked
    unless @locked
      # End method
      return
    end
    # Clear locked flag
    @locked = false
    # If direction is not fixed
    unless @direction_fix
      # If prelock direction is saved
      if @prelock_direction != 0
        # Restore prelock direction
        @direction = @prelock_direction
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Move to Designated Position
  #     x : x-coordinate
  #     y : y-coordinate
  #--------------------------------------------------------------------------
  def moveto(x, y)
    @x = x % $game_map.width
    @y = y % $game_map.height
    @real_x = @x * 128
    @real_y = @y * 128
    @prelock_direction = 0
  end
  #--------------------------------------------------------------------------
  # * Get Screen X-Coordinates
  #--------------------------------------------------------------------------
  def screen_x
    # Get screen coordinates from real coordinates and map display position
    return (@real_x - $game_map.display_x + 3) / 4 + 16
  end
  #--------------------------------------------------------------------------
  # * Get Screen Y-Coordinates
  #--------------------------------------------------------------------------
  def screen_y
    # Get screen coordinates from real coordinates and map display position
    y = (@real_y - $game_map.display_y + 3) / 4 + 32
    # Make y-coordinate smaller via jump count
    if @jump_count >= @jump_peak
      n = @jump_count - @jump_peak
    else
      n = @jump_peak - @jump_count
    end
    return y - (@jump_peak * @jump_peak - n * n) / 2
  end
  #--------------------------------------------------------------------------
  # * Get Screen Z-Coordinates
  #     height : character height
  #--------------------------------------------------------------------------
  def screen_z(height = 0)
    # If display flag on closest surface is ON
    if @always_on_top
      # 999, unconditional
      return 999
    end
    if @custom_flags.include? :bottom
      return 0
    end
    # Get screen coordinates from real coordinates and map display position
    z = (@real_y - $game_map.display_y + 3) / 4 + 32
    # If tile
    if @tile_id > 0
      # Add tile priority * 32
      return z + $game_map.priorities[@tile_id] * 32
    # If character
    else
      # If height exceeds 32, then add 31
      return z + ((height > 32) ? 31 : 0)
    end
  end
  #--------------------------------------------------------------------------
  # * Get Thicket Depth
  #--------------------------------------------------------------------------
  def bush_depth
    # If tile, or if display flag on the closest surface is ON
    if @tile_id > 0 or @always_on_top
      return 0
    end
    # If element tile other than jumping, then 12; anything else = 0
    if @jump_count == 0 and $game_map.bush?(@x, @y)
      return 12
    else
      return 0
    end
  end
  #--------------------------------------------------------------------------
  # * Get Terrain Tag
  #--------------------------------------------------------------------------
  def terrain_tag
    return $game_map.terrain_tag(@x, @y)
  end
end
