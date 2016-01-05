#==============================================================================
# ** Sprite_Character
#------------------------------------------------------------------------------
#  This sprite is used to display the character.It observes the Game_Character
#  class and automatically changes sprite conditions.
#==============================================================================

class Sprite_Character
  #--------------------------------------------------------------------------
  # * Public Instance Variables
  #--------------------------------------------------------------------------
  attr_accessor :character                # character
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     viewport       : viewport
  #     light_viewport : light viewport
  #     character      : character (Game_Character)
  #--------------------------------------------------------------------------
  def initialize(viewport, light_viewport, character = nil)
    @sprite = RPG::Sprite.new(viewport)
    @light_sprite = RPG::Sprite.new(light_viewport)
    @light_sprite.blend_type = 1
    @character = character
    update
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    # Update sprites
    @sprite.update
    @light_sprite.update
    # If tile ID, file name, or hue are different from current ones
    if @tile_id != @character.tile_id or
       @character_name != @character.character_name or
       @character_hue != @character.character_hue
      # Remember tile ID, file name, and hue
      @tile_id = @character.tile_id
      @character_name = @character.character_name
      @character_hue = @character.character_hue
      # If tile ID value is valid
      if @tile_id >= 384
        @sprite.bitmap = RPG::Cache.tile($game_map.tileset_name,
          @tile_id, @character.character_hue)
        @light_sprite.visible = false
        @sprite.src_rect.set(0, 0, 32, 32)
        @light_sprite.src_rect.set(0, 0, 32, 32)
        self.ox = 16
        self.oy = 32
      # If tile ID value is invalid
      else
        @sprite.bitmap = RPG::Cache.character(@character.character_name,
          @character.character_hue)
        begin
         @light_sprite.bitmap = RPG::Cache.lightmap(@character.character_name)
         @light_sprite.visible = true
        rescue
         @light_sprite.bitmap = nil
         @light_sprite.visible = false
        end
        @cw = @sprite.bitmap.width / 4
        @ch = @sprite.bitmap.height / 4
        self.ox = @cw / 2
        self.oy = @ch
      end
    end
    # Set visible situation
    self.visible = (not @character.transparent)
    # If graphic is character
    if @tile_id == 0
      # Set rectangular transfer
      sx = @character.pattern * @cw
      sy = (@character.direction - 2) / 2 * @ch
      @sprite.src_rect.set(sx, sy, @cw, @ch)
      @light_sprite.src_rect.set(sx, sy, @cw, @ch)
    end
    # Set sprite coordinates
    self.x = @character.screen_x
    self.y = @character.screen_y
    self.z = @character.screen_z(@ch)
    # Set opacity level, blend method, and bush depth
    self.opacity = @character.opacity
    self.blend_type = @character.blend_type
    self.bush_depth = @character.bush_depth
    # Animation
    if @character.animation_id != 0
      animation = $data_animations[@character.animation_id]
      animation(animation, true)
      @character.animation_id = 0
    end
  end
  #--------------------------------------------------------------------------
  # * Sprite attribute wrapper
  #--------------------------------------------------------------------------
  def self.sprite_attr(*args)
    args.each do |arg|
      class_eval("def #{arg};@sprite.#{arg};end")
      class_eval("def #{arg}=(val);@sprite.#{arg}=val;@light_sprite.#{arg}=val;end")
    end
  end
  #--------------------------------------------------------------------------
  # * RPG::Sprite methods
  #--------------------------------------------------------------------------
  def whiten
    @sprite.whiten
    @light_sprite.whiten
  end
  def appear
    @sprite.appear
    @light_sprite.appear
  end
  def escape
    @sprite.escape
    @light_sprite.escape
  end
  def collapse
    @sprite.collapse
    @light_sprite.collapse
  end
  def damage(value, critical)
    @sprite.damage(value, critical)
  end
  def animation(animation, hit)
    @sprite.animation(animation, hit)
  end
  def loop_animation(animation, hit)
    @sprite.loop_animation(animation)
  end
  def blink_on
    @sprite.blink_on
    @light_sprite.blink_on
  end
  def blink_off
    @sprite.blink_off
    @light_sprite.blink_off
  end
  def blink?
    @sprite.blink?
  end
  def effect?
    @sprite.effect?
  end
  #--------------------------------------------------------------------------
  # * Sprite methods
  #--------------------------------------------------------------------------
  def dispose
    @sprite.dispose
    @light_sprite.dispose
    @sprite = nil
    @light_sprite = nil
  end
  def disposed?
    @sprite == nil
  end
  def flash(color, duration)
    @sprite.flash(color, duration)
    @light_sprite.flash(color, duration)
  end
  #--------------------------------------------------------------------------
  # * Sprite properties
  #--------------------------------------------------------------------------
  sprite_attr :src_rect
  sprite_attr :visible
  sprite_attr :x, :y, :z
  sprite_attr :ox, :oy
  sprite_attr :zoom_x, :zoom_y
  sprite_attr :angle
  sprite_attr :mirror
  sprite_attr :bush_depth
  sprite_attr :opacity
  sprite_attr :color
  sprite_attr :tone
  def blend_type
    @sprite.blend_type
  end
  def blend_type=(val)
    @sprite.blend_type = val
  end
end
