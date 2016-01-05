#==============================================================================
# ** Game_Follower
#------------------------------------------------------------------------------
#  This class handles player followers.
#==============================================================================

class Game_Follower < Game_Character
  attr_accessor :leader

  def initialize(leader, actor)
    super()
    @leader = leader
    self.actor = actor
    moveto(leader.x, leader.y)
  end

  # Overrides
  def moveto(x, y)
    @lox = x
    @loy = y
    super
  end
  def passable?(x, y, d)
    return true
  end
  def check_event_trigger_here(triggers)
    false
  end
  def check_event_trigger_there(triggers)
    false
  end
  def check_event_trigger_touch(x, y)
    false
  end

  # Change actor
  def actor
    @actor
  end
  def actor=(actor)
    @actor = actor
    if actor == nil
      @character_name = ''
      @character_hue = 0
    else
      # Set character file name and hue
      @character_name = actor.character_name
      @character_hue = actor.character_hue
      # Initialize opacity level and blending method
      @opacity = 255
      @blend_type = 0
    end
  end

  def update
    @move_speed = $game_player.move_speed
    unless moving?
      if @lox != @leader.x || @loy != @leader.y
        ox = @lox - @x
        oy = @loy - @y
        if ox < 0
          move_left
        elsif ox > 0
          move_right
        elsif oy < 0
          move_up
        elsif oy > 0
          move_down
        end
        @lox = @leader.x
        @loy = @leader.y
      end
    end
    super
  end
end
