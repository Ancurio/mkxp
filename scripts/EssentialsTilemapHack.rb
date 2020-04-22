# ======================================================================
# TILEMAP VERTICAL WRAPPER
#
# This is a little fix for Pokemon Essentials' custom tilemap code
# that works around MKXP's GPU texture size limit that would normally
# stop you from playing a lot of games.
#
# The concept is simple enough: If your tileset is too big, a new
# bitmap will be constructed with all the excess pixels sent to the
# image's right side. This basically means that you now have a limit
# far higher than you should ever actually need.
#
# Because of the extra math the game will have to do to find the right
# pixels, this will probably cause a performance hit while on these
# maps which would normally be megasurfaces.
#
# It can probably be improved by changing CustomTilemap.getRegularTile
# to just find the correct pixels on its own, without having to
# translate the coordinates afterwards, but this will satisfy me just
# for the moment.
#
# Really, it'd be far better to cut up the image on the C++ end and
# use a custom shader to get the image right or something like that,
# but that's work for another day.
#
# For now, I'm just happy I can finally test whatever game I like.
#
$GL_TEX_CAP = 16384 # << This should be automatically set at some point.
#
#                             ~Zoro
#=======================================================================

module VWrap

  def self.clamp(val, min, max)
    val = max if val > max
    val = min if val < min
    return val
  end

  def self.makeVWrappedTileset(originalbmp)
    width = originalbmp.width
    height = originalbmp.height
    if width == 256 && height > $GL_TEX_CAP
      columns = (height / $GL_TEX_CAP.to_f).ceil
      
      return nil if columns * 256 > $GL_TEX_CAP
      bmp = Bitmap.new(256*columns, $GL_TEX_CAP)
      remainder = height % $GL_TEX_CAP
      
      columns.times{|col|
        srcrect = Rect.new(0, col * $GL_TEX_CAP, width, (col + 1 == columns) ? remainder : $GL_TEX_CAP)
        bmp.blt(col*256, 0, originalbmp, srcrect)
      }
      return bmp
    end

    return originalbmp
  end

  def self.blitVWrappedPixels(destX, destY, dest, src, srcrect)
    merge = (srcrect.y % $GL_TEX_CAP) > ((srcrect.y + srcrect.height) % $GL_TEX_CAP)

    srcrect.x = clamp(srcrect.x, 0,256)
    srcrect.width = clamp(srcrect.width, 0, 256 - srcrect.x)
    col = (srcrect.y / $GL_TEX_CAP.to_f).floor
    srcX = col * 256 + srcrect.x
    srcY = srcrect.y % $GL_TEX_CAP

    if !merge
      dest.blt(destX, destY, src, Rect.new(srcX, srcY, srcrect.width, srcrect.height))
    else
      #FIXME won't work on heights longer than two columns, but nobody should need
      # more than 32k pixels high at once anyway
      side = {:a => $GL_TEX_CAP - srcY, :b => srcrect.height - ($GL_TEX_CAP - srcY)}
      dest.blt(destX, destY, src, Rect.new(srcX, srcY, srcrect.width, side[:a]))
      dest.blt(destX, destY + side[:a], src, Rect.new(srcX + 256, 0, srcrect.width, side[:b]))
    end
  end
end

# -------------------------
if $MKXP == true
  class CustomTilemap
    def tileset=(value)
      if value.height > $GL_TEX_CAP || value.width > $GL_TEX_CAP
        @tileset = VWrap::makeVWrappedTileset(value)
      else
        @tileset = value
      end
      @tilesetchanged = true
    end

    alias old_getRegularTile getRegularTile
    def getRegularTile(sprite, id)
      return old_getRegularTile(sprite, id) if @tileset.width <= 256

      bitmap = @regularTileInfo[id]
      if !bitmap
        bitmap = Bitmap.new(@tileWidth, @tileHeight)
        rect=Rect.new(((id - 384)&7)*@tileSrcWidth,((id - 384)>>3)*@tileSrcHeight,
           @tileSrcWidth,@tileSrcHeight)
           VWrap::blitVWrappedPixels(0,0, bitmap, @tileset, rect)
        @regularTileInfo[id]=bitmap
      end
      sprite.bitmap = bitmap if sprite.bitmap != bitmap
    end
  end
end
