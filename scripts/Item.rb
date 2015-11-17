module Item
  COMBINATIONS = {
    [3, 4] => 5,
    [25, 26] => 30,
  }

  def self.combine(item_a, item_b)
    items = [item_a, item_b].minmax
    return COMBINATIONS[items]
  end
end
