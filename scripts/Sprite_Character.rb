#==============================================================================
# ** Sprite_Character
#------------------------------------------------------------------------------
#  This sprite is used to display the character.It observes the Game_Character
#  class and automatically changes sprite conditions.
#==============================================================================

class Sprite_Character < RPG::Sprite
  #--------------------------------------------------------------------------
  # * Public Instance Variables
  #--------------------------------------------------------------------------
  attr_accessor :character                # character
  #--------------------------------------------------------------------------
  # * Object Initialization
  #     viewport  : viewport
  #     character : character (Game_Character)
  #--------------------------------------------------------------------------
  def initialize(viewport, character = nil)
    super(viewport)
    @character = character
    update
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    super
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
        self.bitmap = RPG::Cache.tile($game_map.tileset_name,
          @tile_id, @character.character_hue)
        self.src_rect.set(0, 0, 32, 32)
        self.ox = 16
        self.oy = 32
      # If tile ID value is invalid
      else
        self.bitmap = RPG::Cache.character(@character.character_name,
          @character.character_hue)
        #begin
        #  self.lightmap = RPG::Cache.lightmap(@character.character_name)
        #rescue
        #  self.lightmap = nil
        #end
        @cw = bitmap.width / 4
        @ch = bitmap.height / 4
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
      self.src_rect.set(sx, sy, @cw, @ch)
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
end
