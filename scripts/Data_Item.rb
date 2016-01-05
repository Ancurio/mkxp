module Item
  COMBINATIONS = {
    [3, 4] => 5, # alcohol + dry branch => wet branch
    [25, 26] => 30, # feather + ink bottle => pen
    [32, 36] => 33, # button + magnets => magnetized button
    [37, 38] => 32, # tin + scissors => button
    [36, 37] => 100, # tin + magnets => can't combine
  }

  def self.combine(item_a, item_b)
    items = [item_a, item_b].minmax
    return COMBINATIONS[items]
  end
end
