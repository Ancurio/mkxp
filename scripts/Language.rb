class Language
  attr_reader :font
  attr_reader :character_table
  attr_reader :character_table_height

  def initialize(font, character_table, character_table_height)
    @font = font
    @character_table = character_table
    @character_table_height = character_table_height
  end

  def self.get(code = :en)
    return LANGUAGES[code] || LANGUAGES[:en]
  end
end
