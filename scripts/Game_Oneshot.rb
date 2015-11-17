class Game_Oneshot
  #--------------------------------------------------------------------------
  # * Public Instance Variables
  #--------------------------------------------------------------------------
  attr_accessor :player_name              # map music (for battle memory)

  def initialize
    user_name = Oneshot::USER_NAME.split(/\s+/)
    if user_name[0].casecmp('the') == 0 || user_name[0].casecmp('a') == 0
      @player_name = user_name.join(' ')
    else
      @player_name = user_name[0]
    end
    @lights = {}
    self.lang = Oneshot::LANG
  end

  # lang
  def lang
    @lang
  end
  def lang=(val)
    @lang = val
    $tr = Translator.new(val)
    $language = Language.get(val)
    Font.default_name = $language.font
    Oneshot.set_yes_no(tr('Yes'), tr('No'))
  end

  # MARSHAL
  def marshal_dump
    [@player_name, @lang]
  end
  def marshal_load(array)
    @player_name, self.lang = array
  end
end
