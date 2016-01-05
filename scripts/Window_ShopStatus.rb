#==============================================================================
# ** Window_ShopStatus
#------------------------------------------------------------------------------
#  This window displays number of items in possession and the actor's equipment
#  on the shop screen.
#==============================================================================

class Window_ShopStatus < Window_Base
  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    super(368, 128, 272, 352)
    self.contents = Bitmap.new(width - 32, height - 32)
    @item = nil
    refresh
  end
  #--------------------------------------------------------------------------
  # * Refresh
  #--------------------------------------------------------------------------
  def refresh
    self.contents.clear
    if @item == nil
      return
    end
    case @item
    when RPG::Item
      number = $game_party.item_number(@item.id)
    when RPG::Weapon
      number = $game_party.weapon_number(@item.id)
    when RPG::Armor
      number = $game_party.armor_number(@item.id)
    end
    self.contents.font.color = system_color
    self.contents.draw_text(4, 0, 200, 32, "number in possession")
    self.contents.font.color = normal_color
    self.contents.draw_text(204, 0, 32, 32, number.to_s, 2)
    if @item.is_a?(RPG::Item)
      return
    end
    # Equipment adding information
    for i in 0...$game_party.actors.size
      # Get actor
      actor = $game_party.actors[i]
      # If equippable, then set to normal text color. If not, set to
      # invalid text color.
      if actor.equippable?(@item)
        self.contents.font.color = normal_color
      else
        self.contents.font.color = disabled_color
      end
      # Draw actor's name
      self.contents.draw_text(4, 64 + 64 * i, 120, 32, actor.name)
      # Get current equipment
      if @item.is_a?(RPG::Weapon)
        item1 = $data_weapons[actor.weapon_id]
      elsif @item.kind == 0
        item1 = $data_armors[actor.armor1_id]
      elsif @item.kind == 1
        item1 = $data_armors[actor.armor2_id]
      elsif @item.kind == 2
        item1 = $data_armors[actor.armor3_id]
      else
        item1 = $data_armors[actor.armor4_id]
      end
      # If equippable
      if actor.equippable?(@item)
        # If weapon
        if @item.is_a?(RPG::Weapon)
          atk1 = item1 != nil ? item1.atk : 0
          atk2 = @item != nil ? @item.atk : 0
          change = atk2 - atk1
        end
        # If armor
        if @item.is_a?(RPG::Armor)
          pdef1 = item1 != nil ? item1.pdef : 0
          mdef1 = item1 != nil ? item1.mdef : 0
          pdef2 = @item != nil ? @item.pdef : 0
          mdef2 = @item != nil ? @item.mdef : 0
          change = pdef2 - pdef1 + mdef2 - mdef1
        end
        # Draw parameter change values
        self.contents.draw_text(124, 64 + 64 * i, 112, 32,
          sprintf("%+d", change), 2)
      end
      # Draw item
      if item1 != nil
        x = 4
        y = 64 + 64 * i + 32
        bitmap = RPG::Cache.icon(item1.icon_name)
        opacity = self.contents.font.color == normal_color ? 255 : 128
        self.contents.blt(x, y + 4, bitmap, Rect.new(0, 0, 24, 24), opacity)
        self.contents.draw_text(x + 28, y, 212, 32, item1.name)
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Set Item
  #     item : new item
  #--------------------------------------------------------------------------
  def item=(item)
    if @item != item
      @item = item
      refresh
    end
  end
end
