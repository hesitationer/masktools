#ifndef __Mt_Lutxy_H__
#define __Mt_Lutxy_H__

#include "../../../common/base/filter.h"
#include "../../../../common/parser/parser.h"
#include <mutex>

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut { namespace Dual {

typedef void(Processor)(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, const Byte lut[65536]);
typedef void(Processor16)(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, const Word *lut);
typedef void(ProcessorCtx)(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc, ptrdiff_t nSrcPitch, int nWidth, int nHeight, Parser::Context *ctx);

Processor lut_c;
extern Processor16 *lut10_c;
extern Processor16 *lut12_c;
extern Processor16 *lut14_c;
extern Processor16 *lut16_c;

ProcessorCtx realtime8_c;
extern ProcessorCtx *realtime10_c;
extern ProcessorCtx *realtime12_c;
extern ProcessorCtx *realtime14_c;
extern ProcessorCtx *realtime16_c;
ProcessorCtx realtime32_c;

//#define METHOD_1

class Lutxy : public MaskTools::Filter
{
  /*
   struct Lut {
     bool used;
     Byte *ptr;
   };

   Lut Luts[4];
   */
   Byte luts[3][65536];
   Word *luts16[3]; // 10+bits

   // for realtime
   std::deque<Filtering::Parser::Symbol> *parsed_expressions[3];

   Processor *processor;
   Processor16 *processor16;
   ProcessorCtx *processorCtx;
   ProcessorCtx *processorCtx16;
   ProcessorCtx *processorCtx32;
   int bits_per_pixel;
   bool isStacked;
   bool realtime;

protected:
    virtual void process(int n, const Plane<Byte> &dst, int nPlane, const ::Filtering::Frame<const Byte> frames[3], const Constraint constraints[3]) override
    {
        UNUSED(n);
        UNUSED(constraints);
        if (realtime) {
          // thread safety
          Parser::Context ctx(*parsed_expressions[nPlane]);
          processorCtx(dst.data(), dst.pitch(), frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(), dst.width(), dst.height(), &ctx);
        }
        else if (bits_per_pixel == 8)
          processor(dst.data(), dst.pitch(), frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(), dst.width(), dst.height(), luts[nPlane]);
        else if (bits_per_pixel <= 16)
          processor16(dst.data(), dst.pitch(), frames[0].plane(nPlane).data(), frames[0].plane(nPlane).pitch(), dst.width(), dst.height(), luts16[nPlane]);
    }

public:
   Lutxy(const Parameters &parameters) : MaskTools::Filter( parameters, FilterProcessingType::INPLACE )
   {
      for (int i = 0; i < 3; i++) {
        luts16[i] = nullptr;
        parsed_expressions[i] = nullptr;
      }

      static const char *expr_strs[] = { "yExpr", "uExpr", "vExpr" };

      Parser::Parser parser = Parser::getDefaultParser().addSymbol(Parser::Symbol::X).addSymbol(Parser::Symbol::Y);

      bits_per_pixel = bit_depths[C];
      int max_pixel_value = (1 << bits_per_pixel) - 1;
      realtime = parameters["realtime"].toBool();

      // 14, 16 bit and float: default realtime, 
      // hardcore users on x64 with 8GB memory and plenty of time can override 16 bit lutxy with realtime=false :)
      // lut sizes
      // 10 bits: 2 MBytes (2*1024*1024) per expression 
      // 12 bits: 32 MBytes (2*4096*4096) per expression 
      // 14 bits: 512 MBytes (2*16384*16384) per expression 
      // 16 bits: 8 GBytes (2*65536*65536) per expression 
      if (bits_per_pixel >= 14 && !parameters["realtime"].is_defined()) {
        realtime = true;
      }

      if (bits_per_pixel == 32)
        realtime = true;

      if (bits_per_pixel == 16) {
        if ((uint64_t)std::numeric_limits<size_t>::max() <= 0xFFFFFFFFull && bits_per_pixel == 16) {
          realtime = true; // not even possible a real 16 bit lutxy on 32 bit environment
        }
      }

      /* compute the luts */
      for ( int i = 0; i < 3; i++ )
      {
          if (operators[i] != PROCESS) {
              continue;
          }

          if (parameters[expr_strs[i]].undefinedOrEmptyString() && parameters["expr"].undefinedOrEmptyString()) {
              operators[i] = NONE; //inplace
              continue;
          }

          if (parameters[expr_strs[i]].is_defined())
            parser.parse(parameters[expr_strs[i]].toString(), " ");
          else
            parser.parse(parameters["expr"].toString(), " ");

          Parser::Context ctx(parser.getExpression());

          if (!ctx.check())
          {
            error = "invalid expression in the lut";
            return;
          }

          if (realtime) {
            parsed_expressions[i] = new std::deque<Parser::Symbol>(parser.getExpression());

            switch (bits_per_pixel) {
            case 8: processorCtx = realtime8_c; break;
            case 10: processorCtx = realtime10_c; break;
            case 12: processorCtx = realtime12_c; break;
            case 14: processorCtx = realtime14_c; break;
            case 16: processorCtx = realtime16_c; break;
            case 32: processorCtx = realtime32_c; break;
            }
            continue;
          }

          // pure lut, no realtime

          if (bits_per_pixel > 8) {
            size_t buffer_size = ((size_t)1 << bits_per_pixel) * ((size_t)1 << bits_per_pixel) * sizeof(uint16_t);
            luts16[i] = reinterpret_cast<Word*>(_aligned_malloc(buffer_size, 16));
          }

          switch (bits_per_pixel) {
          case 8: 
            for (int x = 0; x < 256; x++)
              for (int y = 0; y < 256; y++)
                luts[i][(x << 8) + y] = ctx.compute_byte(x, y);
            processor = lut_c;
            break;
          case 10:
            for (int x = 0; x < 1024; x++)
              for (int y = 0; y < 1024; y++)
                luts16[i][(x << 10) + y] = min(ctx.compute_word(x, y),(Word)max_pixel_value);
            processor16 = lut10_c;
            break;
          case 12:
            for (int x = 0; x < 4096; x++)
              for (int y = 0; y < 4096; y++)
                luts16[i][(x << 12) + y] = min(ctx.compute_word(x, y), (Word)max_pixel_value);
            processor16 = lut12_c;
            break;
          case 14:
            for (int x = 0; x < 16384; x++)
              for (int y = 0; y < 16384; y++)
                luts16[i][(x << 14) + y] = min(ctx.compute_word(x, y), (Word)max_pixel_value);
            processor16 = lut14_c;
            break;
          case 16:
            // 64bit only
            for (int x = 0; x < 65536; x++)
              for (int y = 0; y < 65536; y++)
                luts16[i][((size_t)x << 16) + y] = ctx.compute_word(x, y);
            processor16 = lut16_c;
            break;
          }
      }
   }

   ~Lutxy()
   {
     for (int i = 0; i < 3; i++) {
       _aligned_free(luts16[i]);
       delete parsed_expressions[i];
     }
   }

   InputConfiguration &input_configuration() const { return InPlaceTwoFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_lutxy";

      signature.add(Parameter(TYPE_CLIP, ""));
      signature.add(Parameter(TYPE_CLIP, ""));
      signature.add(Parameter(String("x"), "expr"));
      signature.add(Parameter(String("x"), "yExpr"));
      signature.add(Parameter(String("x"), "uExpr"));
      signature.add(Parameter(String("x"), "vExpr"));
      signature.add(Parameter(false, "realtime"));

      return add_defaults( signature );
   }
};

} } } } }

#endif