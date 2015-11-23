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

  LANGUAGES = {
    :en => Language.new(
      'Terminus (TTF)', [
        "A","B","C","D","E",
        "F","G","H","I","J",
        "K","L","M","N","O",
        "P","Q","R","S","T",
        "U","V","W","X","Y",
        "Z"," "," "," "," ",
        "+","-","*","/","!",
        "1","2","3","4","5",

        "a","b","c","d","e",
        "f","g","h","i","j",
        "k","l","m","n","o",
        "p","q","r","s","t",
        "u","v","w","x","y",
        "z"," "," "," "," ",
        "#","$","%","&","@",
        "6","7","8","9","0",
      ], 8
    ),
    :es => Language.new(
      'Terminus (TTF)', [
        "A","B","C","D","E",
        "F","G","H","I","J",
        "K","L","M","N","O",
        "P","Q","R","S","T",
        "U","V","W","X","Y",
        "Z"," ","Ñ","Ü"," ",
        "Á","É","Í","Ó","Ú",
        "+","-","*","/","!",
        "1","2","3","4","5",

        "a","b","c","d","e",
        "f","g","h","i","j",
        "k","l","m","n","o",
        "p","q","r","s","t",
        "u","v","w","x","y",
        "z"," ","ñ","ü"," ",
        "á","é","í","ó","ú",
        "¡","#","%","&","@",
        "6","7","8","9","0",
      ], 9
    ),
    :ja => Language.new(
      'WenQuanYi Micro Hei', [
        "あ","い","う","え","お",
        "か","き","く","け","こ",
        "さ","し","す","せ","そ",
        "た","ち","つ","て","と",
        "な","に","ぬ","ね","の",
        "は","ひ","ふ","へ","ほ",
        "ま","み","む","め","も",
        "や", "","ゆ", "","よ",
        "ら","り","る","れ","ろ",

        "わ", "","を", "","ん",
        "が","ぎ","ぐ","げ","ご",
        "ざ","じ","ず","ぜ","ぞ",
        "だ","ぢ","づ","で","ど",
        "ば","び","ぶ","べ","ぼ",
        "ぱ","ぴ","ぷ","ぺ","ぽ",
        "ゃ","ゅ","ょ","っ","ゎ",
        "ぁ","ぃ","ぅ","ぇ","ぉ",
        "ー","・", "", "", "",

        "ア","イ","ウ","エ","オ",
        "カ","キ","ク","ケ","コ",
        "サ","シ","ス","セ","ソ",
        "タ","チ","ツ","テ","ト",
        "ナ","ニ","ヌ","ネ","ノ",
        "ハ","ヒ","フ","ヘ","ホ",
        "マ","ミ","ム","メ","モ",
        "ヤ", "","ユ", "","ヨ",
        "ラ","リ","ル","レ","ロ",

        "ワ", "","ヲ", "","ン",
        "ガ","ギ","グ","ゲ","ゴ",
        "ザ","ジ","ズ","ゼ","ゾ",
        "ダ","ヂ","ヅ","デ","ド",
        "バ","ビ","ブ","ベ","ボ",
        "パ","ピ","プ","ペ","ポ",
        "ャ","ュ","ョ","ッ","ヮ",
        "ァ","ィ","ゥ","ェ","ォ",
        "ー","・","ヴ", "", "",
      ], 9
    ),
    :ko => Language.new(
      'WenQuanYi Micro Hei', [
        "가","개","갸","거","게",
        "겨","고","교","구","규",
        "그","기","나","내","냐",
        "너","네","녀","노","뇨",
        "누","뉴","느","니","다",
        "대","댜","더","데","뎌",
        "도","됴","두","듀","드",
        "디","라","래","랴","러",

        "레","려","로","료","루",
        "류","르","리","마","매",
        "먀","머","메","며","모",
        "묘","무","뮤","므","미",
        "바","배","뱌","버","베",
        "벼","보","뵤","부","뷰",
        "브","비","아","애","야",
        "어","에","여","오","요",

        "우","유","으","이","자",
        "재","쟈","저","제","져",
        "조","죠","주","쥬","즈",
        "지","차","채","챠","처",
        "체","쳐","초","쵸","추",
        "츄","츠","치","카","캐",
        "캬","커","케","켜","코",
        "쿄","쿠","큐","타","태",

        "탸","터","테","텨","토",
        "툐","투","튜","트","티",
        "파","패","퍄","퍼","페",
        "펴","포","표","푸","퓨",
        "프","피","하","해","햐",
        "허","헤","혀","호","효",
        "후","휴","흐","히","",
        "","","","","",
      ], 8
    ),
  }
end
