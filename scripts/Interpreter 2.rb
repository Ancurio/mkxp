#==============================================================================
# ** Interpreter (part 2)
#------------------------------------------------------------------------------
#  This interpreter runs event commands. This class is used within the
#  Game_System class and the Game_Event class.
#==============================================================================

class Interpreter
  #--------------------------------------------------------------------------
  # * Event Command Execution
  #--------------------------------------------------------------------------
  def execute_command
    # If last to arrive for list of event commands
    if @index >= @list.size - 1
      # End event
      command_end
      # Continue
      return true
    end
    # Make event command parameters available for reference via @parameters
    @parameters = @list[@index].parameters
    # Branch by command code
    case @list[@index].code
    when 101  # Show Text
      return command_101
    when 102  # Show Choices
      return command_102
    when 402  # When [**]
      return command_402
    when 403  # When Cancel
      return command_403
    when 103  # Input Number
      return command_103
    when 104  # Change Text Options
      return command_104
    when 105  # Button Input Processing
      return command_105
    when 106  # Wait
      return command_106
    when 111  # Conditional Branch
      return command_111
    when 411  # Else
      return command_411
    when 112  # Loop
      return command_112
    when 413  # Repeat Above
      return command_413
    when 113  # Break Loop
      return command_113
    when 115  # Exit Event Processing
      return command_115
    when 116  # Erase Event
      return command_116
    when 117  # Call Common Event
      return command_117
    when 118  # Label
      return command_118
    when 119  # Jump to Label
      return command_119
    when 121  # Control Switches
      return command_121
    when 122  # Control Variables
      return command_122
    when 123  # Control Self Switch
      return command_123
    when 124  # Control Timer
      return command_124
    when 125  # Change Gold
      return command_125
    when 126  # Change Items
      return command_126
    when 127  # Change Weapons
      return command_127
    when 128  # Change Armor
      return command_128
    when 129  # Change Party Member
      return command_129
    when 131  # Change Windowskin
      return command_131
    when 132  # Change Battle BGM
      return command_132
    when 133  # Change Battle End ME
      return command_133
    when 134  # Change Save Access
      return command_134
    when 135  # Change Menu Access
      return command_135
    when 136  # Change Encounter
      return command_136
    when 201  # Transfer Player
      return command_201
    when 202  # Set Event Location
      return command_202
    when 203  # Scroll Map
      return command_203
    when 204  # Change Map Settings
      return command_204
    when 205  # Change Fog Color Tone
      return command_205
    when 206  # Change Fog Opacity
      return command_206
    when 207  # Show Animation
      return command_207
    when 208  # Change Transparent Flag
      return command_208
    when 209  # Set Move Route
      return command_209
    when 210  # Wait for Move's Completion
      return command_210
    when 221  # Prepare for Transition
      return command_221
    when 222  # Execute Transition
      return command_222
    when 223  # Change Screen Color Tone
      return command_223
    when 224  # Screen Flash
      return command_224
    when 225  # Screen Shake
      return command_225
    when 231  # Show Picture
      return command_231
    when 232  # Move Picture
      return command_232
    when 233  # Rotate Picture
      return command_233
    when 234  # Change Picture Color Tone
      return command_234
    when 235  # Erase Picture
      return command_235
    when 236  # Set Weather Effects
      return command_236
    when 241  # Play BGM
      return command_241
    when 242  # Fade Out BGM
      return command_242
    when 245  # Play BGS
      return command_245
    when 246  # Fade Out BGS
      return command_246
    when 247  # Memorize BGM/BGS
      return command_247
    when 248  # Restore BGM/BGS
      return command_248
    when 249  # Play ME
      return command_249
    when 250  # Play SE
      return command_250
    when 251  # Stop SE
      return command_251
    when 301  # Battle Processing
      return command_301
    when 601  # If Win
      return command_601
    when 602  # If Escape
      return command_602
    when 603  # If Lose
      return command_603
    when 302  # Shop Processing
      return command_302
    when 303  # Name Input Processing
      return command_303
    when 311  # Change HP
      return command_311
    when 312  # Change SP
      return command_312
    when 313  # Change State
      return command_313
    when 314  # Recover All
      return command_314
    when 315  # Change EXP
      return command_315
    when 316  # Change Level
      return command_316
    when 317  # Change Parameters
      return command_317
    when 318  # Change Skills
      return command_318
    when 319  # Change Equipment
      return command_319
    when 320  # Change Actor Name
      return command_320
    when 321  # Change Actor Class
      return command_321
    when 322  # Change Actor Graphic
      return command_322
    when 331  # Change Enemy HP
      return command_331
    when 332  # Change Enemy SP
      return command_332
    when 333  # Change Enemy State
      return command_333
    when 334  # Enemy Recover All
      return command_334
    when 335  # Enemy Appearance
      return command_335
    when 336  # Enemy Transform
      return command_336
    when 337  # Show Battle Animation
      return command_337
    when 338  # Deal Damage
      return command_338
    when 339  # Force Action
      return command_339
    when 340  # Abort Battle
      return command_340
    when 351  # Call Menu Screen
      return command_351
    when 352  # Call Save Screen
      return command_352
    when 353  # Game Over
      return command_353
    when 354  # Return to Title Screen
      return command_354
    when 355  # Script
      return command_355
    else      # Other
      return true
    end
  end
  #--------------------------------------------------------------------------
  # * End Event
  #--------------------------------------------------------------------------
  def command_end
    # Clear list of event commands
    @list = nil
    # If main map event and event ID are valid
    if @main and @event_id > 0
      # Unlock event
      $game_map.events[@event_id].unlock
    end
  end
  #--------------------------------------------------------------------------
  # * Command Skip
  #--------------------------------------------------------------------------
  def command_skip
    # Get indent
    indent = @list[@index].indent
    # Loop
    loop do
      # If next event command is at the same level as indent
      if @list[@index+1].indent == indent
        # Continue
        return true
      end
      # Advance index
      @index += 1
    end
  end
  #--------------------------------------------------------------------------
  # * Get Character
  #     parameter : parameter
  #--------------------------------------------------------------------------
  def get_character(parameter)
    # Branch by parameter
    case parameter
    when -1  # player
      return $game_player
    when 0  # this event
      events = $game_map.events
      return events == nil ? nil : events[@event_id]
    else  # specific event
      events = $game_map.events
      return events == nil ? nil : events[parameter]
    end
  end
  #--------------------------------------------------------------------------
  # * Calculate Operated Value
  #     operation    : operation
  #     operand_type : operand type (0: invariable 1: variable)
  #     operand      : operand (number or variable ID)
  #--------------------------------------------------------------------------
  def operate_value(operation, operand_type, operand)
    # Get operand
    if operand_type == 0
      value = operand
    else
      value = $game_variables[operand]
    end
    # Reverse sign of integer if operation is [decrease]
    if operation == 1
      value = -value
    end
    # Return value
    return value
  end
end
