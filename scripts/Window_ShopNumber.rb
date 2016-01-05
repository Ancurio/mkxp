#==============================================================================
# ** Window_ShopNumber
#------------------------------------------------------------------------------
#  This window is for inputting quantity of items to buy or sell on the
#  shop screen.
#==============================================================================

class Window_ShopNumber < Window_Base
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    super(0, 128, 368, 352)
    self.contents = Bitmap.new(width - 32, height - 32)
    @item = nil
    @max = 1
    @price = 0
    @number = 1
  end
  #--------------------------------------------------------------------------
  # * Set Items, Max Quantity, and Price
  #--------------------------------------------------------------------------
  def set(item, max, price)
    @item = item
    @max = max
    @price = price
    @number = 1
    refresh
  end
  #--------------------------------------------------------------------------
  # * Set Inputted Quantity
  #--------------------------------------------------------------------------
  def number
    return @number
  end
  #--------------------------------------------------------------------------
  # * Refresh
  #--------------------------------------------------------------------------
  def refresh
    self.contents.clear
    draw_item_name(@item, 4, 96)
    self.contents.font.color = normal_color
    self.contents.draw_text(272, 96, 32, 32, "×")
    self.contents.draw_text(308, 96, 24, 32, @number.to_s, 2)
    self.cursor_rect.set(304, 96, 32, 32)
    # Draw total price and currency units
    domination = $data_system.words.gold
    cx = contents.text_size(domination).width
    total_price = @price * @number
    self.contents.font.color = normal_color
    self.contents.draw_text(4, 160, 328-cx-2, 32, total_price.to_s, 2)
    self.contents.font.color = system_color
    self.contents.draw_text(332-cx, 160, cx, 32, domination, 2)
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    super
    if self.active
      # Cursor right (+1)
      if Input.repeat?(Input::RIGHT) and @number < @max
        $game_system.se_play($data_system.cursor_se)
        @number += 1
        refresh
      end
      # Cursor left (-1)
      if Input.repeat?(Input::LEFT) and @number > 1
        $game_system.se_play($data_system.cursor_se)
        @number -= 1
        refresh
      end
      # Cursdr up (+10)
      if Input.repeat?(Input::UP) and @number < @max
        $game_system.se_play($data_system.cursor_se)
        @number = [@number + 10, @max].min
        refresh
      end
      # Cursor down (-10)
      if Input.repeat?(Input::DOWN) and @number > 1
        $game_system.se_play($data_system.cursor_se)
        @number = [@number - 10, 1].max
        refresh
      end
    end
  end
end
