### MaskTools 2 ###

**Masktools2 v2.2.5 (20170323)**

mod by pinterf

Differences to Masktools 2.0b1

- project moved to Visual Studio 2015 Update 3
  Requires VS2015 Update 3 redistributables
- Fix: mt_merge (and probably other multi-clip filters) may result in corrupted results 
  under specific circumstances, due to using video frame pointers which were already released from memory
- no special function names for high bit depth filters
- filters are auto registering their mt mode as MT_NICE_FILTER for Avisynth+
- Avisynth+ high bit depth support (incl. planar RGB, but color spaces with alpha plane are not yet supported)
  All filters are now supporting 10, 12, 14, 16 bits and float
  Threshold and sc_value parameters are scaled automatically to the current bit depth (v2.2.5-) from a default 8-bit value.
  Default range of such parameters can be overridden to 8-16 bits or float.
- YV411 (8 bit 4:1:1) support
- mt_merge accepts 4:2:2 clips when luma=true (8-16 bit)
- mt_merge accepts 4:1:1 clips when luma=true
- mt_merge to discard U and V automatically when input is greyscale
- some filters got AVX (float) and AVX2 (integer) support:
  mt_merge: 8-16 bit: AVX2, float:AVX
  mt_logic: 8-16 bit: AVX2, float:AVX
  mt_edge: 32 bit float AVX
- mt_polish to recognize new constants and scaling operator, and some other operators introduced in earlier versions.
  For a complete list, see v2.2.4 change log
- new: mt_lutxyza. Accepts four clips. 4th variable name is 'a' (besides x, y and z)
- new: mt_luts: weight expressions as an addon for then main expression(s) (martin53's idea)
    - wexpr
    - ywExpr, uwExpr, vwExpr
  If the relevant parameter strings exist, the weighting expression is evaluated
  for each source/neighborhood pixel values (lut or realtime, depending on the bit depth and the "realtime" parameter). 
  Then the usual lut result is premultiplied by this weight factor before it gets accumulated. 
  
  Weights are float values. Weight luts are x,y (2D) luts, similarly to the base working mode,
  where x is the base pixel, y is the current pixel from the neighbourhood, defined in "pixels".
  
  When the weighting expression is "1", the result is the same as the basic weightless mode.
  
  For modes "average" and "std" the weights are summed up. Result is: sum(value_i*weight_i)/sum(weight_i).
  
  When all weights are equal to 1.0 then the expression will result in the average: sum(value_i)/n.
  
  Same logic works for min/max/median/etc., the "old" lut values are pre-multiplied with the weights before accumulation.

- expression syntax supporting bit depth independent expressions
  - bit-depth aware scale operators
  
      operator "@B" or "scaleb" scales from 8 bit to current bit depth using bit-shifts
                  Use this for YUV. "235 @B" or "235 scaleb" -> always results in max luma
                  
      operator "@F" or "scalef" scales from 8 bit to current bit depth using full range stretch
                  "255 @F" or "255 scalef" results in maximum pixel value of current bit depth.
                  Calculation: x/255*65535 for a 8->16 bit sample (rgb)
                  
  - hints for non-8 bit based constants: 
      Added configuration keywords i8, i10, i12, i14, i16 and f32 in order to tell the expression evaluator 
      the bit depth of the values that are to scale by @B/scaleb and @F/scalef operators.
            
      By default @B and @F scales from 8 bit to the bit depth of the clip.
      
      i8 .. i16 and f32 sets the default conversion base to 8..16 bits or float, respectively.
      
      These keywords can appear anywhere in the expression, but only the last occurence will be effective for the whole expression.
      
      Examples 
```
      8 bit video, no modifier: "x y - 256 @B *" evaluates as "x y - 256 *"
      10 bit video, no modifier: "x y - 256 @B *" evaluates as "x y - 1024 *"
      10 bit video: "i16 x y - 65536 @B *" evaluates as "x y - 1024 *"
      8 bit video: "i10 x y - 512 @B *" evaluates as "x y - 128 *"                  
```

  - new pre-defined, bit depth aware constants
  
    - bitdepth: automatic silent parameter of the lut expression (clip bit depth)
    - sbitdepth: automatic silent parameter of the lut expression (bit depth of values to scale)
    - range_half --> autoscaled 128 or 0.5 for float
    - range_max --> 255/1023/4095/16383/65535 or 1.0 for float
    - range_size --> 256/1024...65536
    - ymin, ymax, cmin, cmax --> 16/235 and 16/240 autoscaled.
      
Example #1 (bit depth dependent, all constants are treated as-is): 
```
      expr8_luma = "x 16 - 219 / 255 *"
      expr10_luma = "x 64 - 876 / 1023 *"
      expr16_luma = "x 4096 - 56064 / 65535 *"
```
      
Example #2 (new, with auto-scale operators )      
```
      expr_luma =  "x 16 @B - 219 @B / 255 @F *"
      expr_chroma =  "x 16 @B - 224 @B / 255 @F *"
```
      
Example #3 (new, with constants)
```
      expr_luma = "x ymin - ymax ymin - / range_max *"
      expr_chroma = "x cmin - cmax cmin - / range_max *"
```
  - new expression syntax: auto scale modifiers for float clips (test for real.finder):
    Keyword at the beginning of the expression:
    - clamp_f_i8, clamp_f_i10, clamp_f_i12, clamp_f_i14 or clamp_f_i16 for scaling and clamping
    - clamp_f_f32 or clamp_f: for clamping the result to 0..1 (floats are not clamped by default!)
  
    Input values 'x', 'y', 'z' and 'a' are autoscaled by 255.0, 1023.0, ... 65535.0 before the expression evaluation,
    so the working range is similar to native 8, 10, ... 16 bits. The predefined constants 'range_max', etc. will behave for 8, 10,..16 bits accordingly.
  
    The result is automatically scaled back to 0..1 _and_ is clamped to that range.
    When using clamp_f_f32 (or clamp_f) the scale factor is 1.0 (so there is no scaling), but the final clamping will be done anyway.
    No integer rounding occurs.

```
    expr = "x y - range_half +"  # good for 8..32 bits but float is not clamped
    expr = "clamp_f y - range_half +"  # good for 8..32 bits and float clamped to 0..1
    expr = "x y - 128 + "  # good for 8 bits
    expr = "clamp_f_i8 x y - 128 +" # good for 8 bits and float, float will be clamped to 0..1
    expr = "clamp_f_i8 x y - range_half +" # good for 8..32 bits, float will be clamped to 0..1
```  

- parameter "stacked" (default false) for filters with stacked format support
  Stacked support is not intentional, but since tp7 did it, I did not remove the feature.
  Filters currently without stacked support will never have it. 
- parameter "realtime" for lut-type filters, slower but at least works on those bit depths
  where LUT tables would occupy too much memory.
  For bit depth limits where realtime = true is set as the default working mode, see table below.
    
  realtime=true can be overridden, one can experiment and force realtime=false even for a 16 bit lutxy 
  (8GBytes lut table!, x64 only) or for 8 bit lutxzya (4GBytes lut table)
   
- parameter "paramscale" for filters working with threshold-like parameters (v2.2.5-)
  Filters: mt_binarize, mt_edge, mt_inpand, mt_expand, mt_inflate, mt_deflate, mt_motion
  paramscale can be "i8" (default), "i10", "i10", "i12", "i14", "i16", "f32" or "none" or ""
  Using "paramscale" tells the filter that parameters are given at what bit depth range.
  By default paramscale is "i8", so existing scripts with parameters in the 0..255 range are working at any bit depths
```  
  mt_binarize(threshold=80*256, paramscale="i16") # threshold is assumed in 16 bit range 
  mt_binarize(threshold=80) # no param: threshold is assumed in 8 bit range
  
  thY1 = 0.1
  thC1 = 0.1
  thY2 = 0.1
  thC2 = 0.1
  paramscale="f32"
  mt_edge(mode="sobel",u=3,v=3,thY1=thY1,thY2=thY2,thC1=thC1,thC2=thC2,paramscale=paramscale) # f32: parameters assumed as float (0..1.0)
```  
   
- Feature matrix   
```
                   8 bit | 10-16 bit | float | stacked | realtime
      mt_invert      X         X         X        -
      mt_binarize    X         X         X        X
      mt_inflate     X         X         X        X
      mt_deflate     X         X         X        X
      mt_inpand      X         X         X        X
      mt_expand      X         X         X        X
      mt_lut         X         X         X        X      when float
      mt_lutxy       X         X         X        -      when bits>=14
      mt_lutxyz      X         X         X        -      when bits>=10
      mt_lutxyza     X         X         X        -      always
      mt_luts        X         X         X        -      when bits>=14 
      mt_lutf        X         X         X        -      when bits>=14   
      mt_lutsx       X         X         X        -      when bits>=10
      mt_lutspa      X         X         X        -
      mt_merge       X         X         X        X
      mt_logic       X         X         X        X
      mt_convolution X         X         X        -
      mt_mappedblur  X         X         X        -
      mt_gradient    X         X         X        -
      mt_makediff    X         X         X        X
      mt_average     X         X         X        X
      mt_adddiff     X         X         X        X
      mt_clamp       X         X         X        X
      mt_motion      X         X         X        -
      mt_edge        X         X         X        -
      mt_hysteresis  X         X         X        -
      mt_infix/mt_polish: available only on non-XP builds
``` 
Masktools2 info:
http://avisynth.nl/index.php/MaskTools2

Forum:
https://forum.doom9.org/showthread.php?t=174333

Article by tp7
http://tp7.github.io/articles/masktools/

Project:
https://github.com/pinterf/masktools/tree/16bit/

Original version: tp7's MaskTools 2 repository.
https://github.com/tp7/masktools/

Changelog
**v2.2.5 (20170323)
- Change #F and #B operators to @B and @F (# is reserved for Avisynth in-string comment character)
- Alias scaleb for @B
- Alias scalef for @F
- New: automatic scaling of parameters (threshold-like, sc_value) from the usual 8 bit range
  Scripts need no extra measures to work for all bit depths with the same "command line"
- New parameter "paramscale" for filters working with threshold-like parameters
  Filters: mt_binarize, mt_edge, mt_inpand, mt_expand, mt_inflate, mt_deflate, mt_motion
  paramscale can be "i8" (default), "i10", "i10", "i12", "i14", "i16", "f32" or "none" or ""
  Using "paramscale" tells the filter that parameters are given at what bit depth range.
  
**v2.2.4 (20170304)
- mt_polish to recognize:
  - new v2.2.x constants and variables
    a, bitdepth, sbitdepth, range_half, range_max, range_size, ymin, ymax, cmin, cmax
  - v2.2.x scaling functions (written as #B(expression) and #F(expression) for mt_polish)
    #B() #F
  - single operand unsigned and signed negate introduced in 2.0.48
    ~u and ~s (written as ~u(expression)and ~s(expression) for mt_polish)
  - other operators introduced in 2.0.48:
    @, &u, |u, °u, @u, >>, >>u, <<, <<u, &s, |s, °s, @s, >>s, <<s
- mt_infix: don't put extra parameter after #F( and #B(
- new expression syntax: auto scale modifiers for float clips (test for real.finder):
  Keyword at the beginning of the expression:
  - clamp_f_i8, clamp_f_i10, clamp_f_i12, clamp_f_i14 or clamp_f_i16 for scaling and clamping
  - clamp_f_f32 or clamp_f: for clamping the result to 0..1
  
  Input values 'x', 'y', 'z' and 'a' are autoscaled by 255.0, 1023.0, ... 65535.0 before the expression evaluation,
  so the working range is similar to native 8, 10, ... 16 bits. The predefined constants 'range_max', etc. will behave for 8, 10,..16 bits accordingly.
  
  The result is automatically scaled back to 0..1 _and_ is clamped to that range.
  When using clamp_f_f32 (or clamp_f) the scale factor is 1.0 (so there is no scaling), but the final clamping will be done anyway.
  No integer rounding occurs.

**v2.2.3 (20170227)**
- mt_logic to 32 bit float (final filter lacking it)
- get CpuInfo from Avisynth (avx/avx2 preparation)
  Note: AVX/AVX2 prequisites
  - recent Avisynth+ which reports extra CPU flags
  - 64 bit OS (but Avisynth can be 32 bits)
  - Windows 7 SP1 or later
- mt_merge: 8-16 bit: AVX2, float:AVX
- mt_logic: 8-16 bit: AVX2, float:AVX
- mt_edge: 10-16 bit and 32 bit float: SSE2/SSE4 optimization
- mt_edge: 32 bit float AVX
- new: mt_luts: weight expressions as an addon for then main expression(s) (martin53's idea)
    - wexpr
    - ywExpr, uwExpr, vwExpr
  If the relevant parameter strings exist, the weighting expression is evaluated
  for each source/neighborhood pixel values (lut or realtime, depending on the bit depth and the "realtime" parameter). 
  Then the usual lut result is premultiplied by this weight factor before it gets accumulated. 

**v2.2.2 (20170223)** completed high bit depth support
- All filters work in 10,12,14,16 bits and float (except mt_logic which is 8-16 only)
- mt_lutxyza 4D lut available with realtime=false! (4 GBytes LUT table, slower initial lut table calculation)
  Allowed only on x64. When is it worth?
  Number of expression evaluations for 4G lut calculation equals to realtime calculation on 
  2100 frames (1920x1080 plane). Be warned, it would take be minutes.
- mt_gradient 10-16 bit / float
- mt_convolution 10-16 bit / float
- mt_motion 10-16 bit / float
- mt_xxpand and mt_xxflate to float
- mt_clamp to float
- mt_merge to float
- mt_binarize to float
- mt_invert to float
- mt_makediff and mt_adddiff to float
- mt_average to float
- Expression syntax supporting bit depth independent expressions: 
  Added configuration keywords i8, i10, i12, i14, i16 and f32 in order to inform the expression evaluator 
  about bit depth of the values that are to scale by #B and #F operators.
 
**v2.2.1 (20170218)** initial high bit depth release

- project moved to Visual Studio 2015 Update 3
  Requires VS2015 Update 3 redistributables
- Fix: mt_merge (and probably other multi-clip filters) may result in corrupted results 
  under specific circumstances, due to using video frame pointers which were already released from memory
- no special function names for high bit depth filters
- filters are auto registering their mt mode as MT_NICE_FILTER for Avisynth+
- Avisynth+ high bit depth support (incl. planar RGB, but color spaces with alpha plane are not yet supported)
  For accepted bit depth, see table below.
- YV411 (8 bit 4:1:1) support
- mt_merge accepts 4:2:2 clips when luma=true (8-16 bit)
- mt_merge accepts 4:1:1 clips when luma=true
- mt_merge to discard U and V automatically when input is greyscale
- new: mt_lutxyza. Accepts four clips. 4th variable name is 'a' (besides x, y and z)
- expression syntax supporting bit depth independent expressions
  - bit-depth aware scale operators #B and #F
  - pre-defined bit depth aware constants
    bitdepth (automatic silent parameter of the lut expression)
    range_half, range_max, range_size, ymin, ymax, cmin, cmax
  - parameter "stacked" (default false) for filters with stacked format support
  - parameter "realtime" for lut-type filters to override default behaviour
  
