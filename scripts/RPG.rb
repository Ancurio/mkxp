# Put any overrides/extensions to RGSS modules here

module RPG
  module Cache
    def self.face(filename)
      self.load_bitmap("Graphics/Faces/", filename)
    end
    def self.menu(filename)
      self.load_bitmap("Graphics/Menus/", filename)
    end
    def self.lightmap(filename)
      self.load_bitmap("Graphics/Lightmaps/", filename)
    end
    def self.light(filename)
      self.load_bitmap("Graphics/Lights/", filename)
    end
  end
end

class Tone
  def +(o)
    Tone.new(self.red + o.red, self.green + o.green, self.blue + o.blue, self.gray + o.gray)
  end

  def *(s)
    Tone.new(self.red * s, self.green * s, self.blue * s, self.gray * s)
  end
end
