#==============================================================================
# ** Spriteset_Map
#------------------------------------------------------------------------------
#  This class brings together map screen sprites, tilemaps, etc.
#  It's used within the Scene_Map class.
#==============================================================================

class Spriteset_Map
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize(spriteset = nil)
    # Make viewports
    @viewport = Viewport.new(0, 0, 640, 480)
    @viewport_bg = Viewport.new(0, 0, 640, 480)
    @viewport_pics = Viewport.new(0, 0, 640, 480)
    @viewport_particles = Viewport.new(0, 0, 640, 480)
    @viewport_lights = Viewport.new(0, 0, 640, 480)
    @viewport_flash = Viewport.new(0, 0, 640, 480)

    @viewport_bg.z = -500
    @viewport_lights.z = 200
    @viewport_pics.z = 500
    @viewport_flash.z = 5000

    # Make tilemap
    @tilemap = Tilemap.new(@viewport)
    if $game_map.tileset_name == "blank"
      @tilemap.tileset = nil
    else
      @tilemap.tileset = RPG::Cache.tileset($game_map.tileset_name)
    end
    for i in 0..6
      autotile_name = $game_map.autotile_names[i]
      @tilemap.autotiles[i] = RPG::Cache.autotile(autotile_name)
    end
    @tilemap.map_data = $game_map.data
    @tilemap.priorities = $game_map.priorities
    # Make panorama plane
    @panorama = Plane.new(@viewport)
    @panorama.z = -1000
    # Make fog plane
    @fog = Plane.new(@viewport)
    @fog.z = 3000
    # Make character sprites
    @character_sprites = []
    for i in $game_map.events.keys.sort
      sprite = Sprite_Character.new(@viewport, @viewport_lights, $game_map.events[i])
      @character_sprites.push(sprite)
    end
    $game_followers.each do |follower|
      @character_sprites.push(Sprite_Character.new(@viewport, @viewport_lights, follower))
    end
    @character_sprites.push(Sprite_Character.new(@viewport, @viewport_lights, $game_player))
    # Make weather
    @weather = RPG::Weather.new(@viewport)
    # Make picture sprites
    @picture_sprites = []
    for i in 1..50
      @picture_sprites.push(Sprite_Picture.new(@viewport_pics,
        $game_screen.pictures[i]))
    end
    # Make timer sprite
    @timer_sprite = Sprite_Timer.new
    # Make lightbulb sprite
    @bulb = Sprite.new(@viewport_lights)
    @bulb.bitmap = RPG::Cache.light('bulb')
    @bulb.opacity = has_lightbulb? ? 255 : 0
    # Frame update
    update
  end
  #--------------------------------------------------------------------------
  # * Dispose
  #--------------------------------------------------------------------------
  def dispose
    # Dispose of tilemap
    @tilemap.tileset.dispose if @tilemap.tileset
    for i in 0..6
      @tilemap.autotiles[i].dispose
    end
    @tilemap.dispose
    # Dispose of panorama plane
    @panorama.dispose
    # Dispose of fog plane
    @fog.dispose
    # Dispose of character sprites
    for sprite in @character_sprites
      sprite.dispose
    end
    # Dispose of weather
    @weather.dispose
    # Dispose of picture sprites
    for sprite in @picture_sprites
      sprite.dispose
    end
    # Dispose of bg
    @bg.dispose if @bg
    # Dispose of particles
    @particles.dispose if @particles
    # Dispose of timer sprite
    @timer_sprite.dispose
    # Dispose of viewports
    @viewport.dispose
    @viewport_pics.dispose
    @viewport_particles.dispose
    @viewport_bg.dispose
    @viewport_lights.dispose
    @viewport_flash.dispose
  end
  #--------------------------------------------------------------------------
  # * Follower operations
  #--------------------------------------------------------------------------
  def add_follower(follower)
    @character_sprites.pop.dispose
    @character_sprites.push(Sprite_Character.new(@viewport, @viewport_lights, follower))
    @character_sprites.push(Sprite_Character.new(@viewport, @viewport_lights, $game_player))
  end
  def remove_follower(follower)
    @character_sprites.reverse_each do |spr|
      if spr.character == follower
        @character_sprites.delete(spr)
        spr.dispose
        break
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    # Update tilemap
    @tilemap.wrapping = $game_map.wrapping
    # If panorama is different from current one
    if @panorama_name != $game_map.panorama_name or
       @panorama_hue != $game_map.panorama_hue
      @panorama_name = $game_map.panorama_name
      @panorama_hue = $game_map.panorama_hue
      if @panorama.bitmap != nil
        @panorama.bitmap.dispose
        @panorama.bitmap = nil
      end
      if @panorama_name != ""
        @panorama.bitmap = RPG::Cache.panorama(@panorama_name, @panorama_hue)
      end
      Graphics.frame_reset
    end
    # If fog is different than current fog
    if @fog_name != $game_map.fog_name or @fog_hue != $game_map.fog_hue
      @fog_name = $game_map.fog_name
      @fog_hue = $game_map.fog_hue
      if @fog.bitmap != nil
        @fog.bitmap.dispose
        @fog.bitmap = nil
      end
      if @fog_name != ""
        @fog.bitmap = RPG::Cache.fog(@fog_name, @fog_hue)
      end
      Graphics.frame_reset
    end
    # If BG is different than current BG
    if @bg_name != $game_map.bg_name
      @bg_name = $game_map.bg_name
      @bg = Sprite.new(@viewport_bg) if !@bg
      if @bg_name.empty?
        @bg.bitmap = nil
      else
        @bg.bitmap = RPG::Cache.panorama(@bg_name, 0)
      end
      Graphics.frame_reset
    end
    # If particles different than current particles
    if @particles_type != $game_map.particles_type
      @particles.dispose if @particles
      @particles_type = $game_map.particles_type
      if @particles_type == nil
        @particles = nil
      else
        case @particles_type
        when :fireflies
          klass = Particle_Firefly
          count = 30
          layer = :front
        else
          raise 'invalid particle type'
        end
        @particles = ParticleLayer.new(@viewport_particles, klass, count)
        @viewport_particles.z = (layer == :front) ? 500 : 50
      end
    end
    # Update bg plane
    @viewport_bg.ox = $game_map.display_x / 4
    @viewport_bg.oy = $game_map.display_y / 4
    # Update tilemap
    @tilemap.ox = $game_map.display_x / 4
    @tilemap.oy = $game_map.display_y / 4
    @tilemap.update
    # Update panorama plane
    if $game_map.clamped_panorama
      x = ($game_player.real_x.to_f / (($game_map.width - 1) * 128)) * (@panorama.bitmap.width - 640)
      y = ($game_player.real_y.to_f / (($game_map.height - 1) * 128)) * (@panorama.bitmap.height - 480)
      @panorama.ox = x < 0.0 ? 0.0 : x
      @panorama.oy = y < 0.0 ? 0.0 : y
    else
      @panorama.ox = $game_map.display_x / 8
      @panorama.oy = $game_map.display_y / 8
    end
    # Update fog plane
    @fog.zoom_x = $game_map.fog_zoom / 100.0
    @fog.zoom_y = $game_map.fog_zoom / 100.0
    @fog.opacity = $game_map.fog_opacity
    @fog.blend_type = $game_map.fog_blend_type
    @fog.ox = $game_map.display_x / 4 + $game_map.fog_ox
    @fog.oy = $game_map.display_y / 4 + $game_map.fog_oy
    @fog.tone = $game_map.fog_tone
    # Update character sprites
    for sprite in @character_sprites
      sprite.update
    end
    # Update weather graphic
    @weather.type = $game_screen.weather_type
    @weather.max = $game_screen.weather_max
    @weather.ox = $game_map.display_x / 4
    @weather.oy = $game_map.display_y / 4
    @weather.update
    # Update picture sprites
    for sprite in @picture_sprites
      sprite.update
    end
    # Update bulb if fading in
    @bulb.tone = $game_map.ambient
    if has_lightbulb?
      if @bulb.opacity < 255
        @bulb.opacity += 2.125
      end
    else
      if @bulb.opacity > 0
        @bulb.opacity -= 2.125
      end
    end
    # Update particles
    @particles.update if @particles
    # Update timer sprite
    @timer_sprite.update
    # Set screen color tone and shake position
    @viewport.tone = $game_screen.tone + $game_map.ambient * (1.0 - @bulb.opacity / 255.0)
    @viewport.ox = $game_screen.shake
    @viewport_lights.ox = $game_screen.shake
    # Set screen flash color
    @viewport_flash.color = $game_screen.flash_color
    # Update viewports
    @viewport.update
    @viewport_flash.update
    @viewport_lights.update
  end
end
