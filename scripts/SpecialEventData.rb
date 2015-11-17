class SpecialEventData
  attr_reader :flags
  attr_reader :collision

  def initialize(flags, collision)
    @flags = flags
    @collision = collision
  end

  def self.get(name)
    return SPECIAL_EVENTS[name]
  end
end
