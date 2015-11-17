#==============================================================================
# ** Sprite_Battler
#------------------------------------------------------------------------------
#  This sprite is used to display the battler.It observes the Game_Character
#  class and automatically changes sprite conditions.
#==============================================================================

class Sprite_Battler < RPG::Sprite
  #--------------------------------------------------------------------------
  # * Public Instance Variables
  #--------------------------------------------------------------------------
  attr_accessor :battler                  # battler
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     viewport : viewport
  #     battler  : battler (Game_Battler)
  #--------------------------------------------------------------------------
  def initialize(viewport, battler = nil)
    super(viewport)
    @battler = battler
    @battler_visible = false
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
    # If battler is nil
    if @battler == nil
      self.bitmap = nil
      loop_animation(nil)
      return
    end
    # If file name or hue are different than current ones
    if @battler.battler_name != @battler_name or
       @battler.battler_hue != @battler_hue
      # Get and set bitmap
      @battler_name = @battler.battler_name
      @battler_hue = @battler.battler_hue
      self.bitmap = RPG::Cache.battler(@battler_name, @battler_hue)
      @width = bitmap.width
      @height = bitmap.height
      self.ox = @width / 2
      self.oy = @height
      # Change opacity level to 0 when dead or hidden
      if @battler.dead? or @battler.hidden
        self.opacity = 0
      end
    end
    # If animation ID is different than current one
    if @battler.damage == nil and
       @battler.state_animation_id != @state_animation_id
      @state_animation_id = @battler.state_animation_id
      loop_animation($data_animations[@state_animation_id])
    end
    # If actor which should be displayed
    if @battler.is_a?(Game_Actor) and @battler_visible
      # Bring opacity level down a bit when not in main phase
      if $game_temp.battle_main_phase
        self.opacity += 3 if self.opacity < 255
      else
        self.opacity -= 3 if self.opacity > 207
      end
    end
    # Blink
    if @battler.blink
      blink_on
    else
      blink_off
    end
    # If invisible
    unless @battler_visible
      # Appear
      if not @battler.hidden and not @battler.dead? and
         (@battler.damage == nil or @battler.damage_pop)
        appear
        @battler_visible = true
      end
    end
    # If visible
    if @battler_visible
      # Escape
      if @battler.hidden
        $game_system.se_play($data_system.escape_se)
        escape
        @battler_visible = false
      end
      # White flash
      if @battler.white_flash
        whiten
        @battler.white_flash = false
      end
      # Animation
      if @battler.animation_id != 0
        animation = $data_animations[@battler.animation_id]
        animation(animation, @battler.animation_hit)
        @battler.animation_id = 0
      end
      # Damage
      if @battler.damage_pop
        damage(@battler.damage, @battler.critical)
        @battler.damage = nil
        @battler.critical = false
        @battler.damage_pop = false
      end
      # Collapse
      if @battler.damage == nil and @battler.dead?
        if @battler.is_a?(Game_Enemy)
          $game_system.se_play($data_system.enemy_collapse_se)
        else
          $game_system.se_play($data_system.actor_collapse_se)
        end
        collapse
        @battler_visible = false
      end
    end
    # Set sprite coordinates
    self.x = @battler.screen_x
    self.y = @battler.screen_y
    self.z = @battler.screen_z
  end
end
