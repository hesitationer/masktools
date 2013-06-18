#ifndef __Mt_Lutxyz_H__
#define __Mt_Lutxyz_H__

#include "../../../common/base/filter.h"
#include "../../../../common/parser/parser.h"
#include "../helpers.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut { namespace Trial {

typedef void(Processor)(Byte *pDst, ptrdiff_t nDstPitch, const Byte *pSrc1, ptrdiff_t nSrc1Pitch, const Byte *pSrc2, ptrdiff_t nSrc2Pitch, int nWidth, int nHeight, const Byte *lut);

Processor lut_c;

class Lutxyz : public MaskTools::Filter
{
   std::pair<bool, Byte*> luts[4];

   static Byte *calculateLut(const std::list<Filtering::Parser::Symbol> &expr) {
       Parser::Context ctx(expr);
       Byte *lut = new Byte[256 * 256 * 256];

       for ( int x = 0; x < 256; x++ ) {
           for ( int y = 0; y < 256; y++ ) {
               for ( int z = 0; z < 256; z++ ) {
                   lut[(x<<16)+(y<<8)+z] = ctx.compute_byte(x, y, z); 
               }
           }
       }
       return lut;
   }

protected:
   virtual void process(int n, const Plane<Byte> &dst, int nPlane)
   {
      UNUSED(n);
      lut_c( dst, dst.pitch(), frames[0].plane(nPlane), frames[0].plane(nPlane).pitch(), frames[1].plane(nPlane), frames[1].plane(nPlane).pitch(), dst.width(), dst.height(), luts[nPlane].second );
   }

public:
   Lutxyz(const Parameters &parameters) : MaskTools::Filter( parameters, FilterProcessingType::INPLACE )
   {
      static const char *expr_strs[] = { "yExpr", "uExpr", "vExpr" };
      
      for (int i = 0; i < 4; ++i) {
          luts[i].first = false;
          luts[i].second = nullptr;
      }

      Parser::Parser parser = Parser::getDefaultParser().addSymbol(Parser::Symbol::X).addSymbol(Parser::Symbol::Y).addSymbol(Parser::Symbol::Z);

      /* compute the luts */
      for ( int i = 0; i < 3; i++ )
      {
          if (operators[i] != PROCESS) {
              continue;
          }

          if (stringValueEmpty(parameters[expr_strs[i]]) && stringValueEmpty(parameters["expr"])) {
              operators[i] = COPY_SECOND; //inplace
              continue;
          }

          if (parameters[expr_strs[i]].is_defined()) {
              parser.parse(parameters[expr_strs[i]].toString(), " ");
              luts[i].first = true;
              luts[i].second = calculateLut(parser.getExpression());
          }
          else {
              if (luts[3].second == nullptr) {
                  parser.parse(parameters["expr"].toString(), " ");
                  luts[3].first = true;
                  luts[3].second = calculateLut(parser.getExpression());
              }
              luts[i].second = luts[3].second;
          }
      }
   }

   ~Lutxyz()
   {
       for (int i = 0; i < 4; ++i) {
           if (luts[i].first) {
               delete[] luts[i].second;
           }
       }
   }

   InputConfiguration &input_configuration() const { return InPlaceThreeFrame(); }

   static Signature filter_signature()
   {
      Signature signature = "mt_lutxyz";

      signature.add(Parameter(TYPE_CLIP, ""));
      signature.add(Parameter(TYPE_CLIP, ""));
      signature.add(Parameter(TYPE_CLIP, ""));
      signature.add(Parameter(String("x"), "expr"));
      signature.add(Parameter(String("x"), "yExpr"));
      signature.add(Parameter(String("x"), "uExpr"));
      signature.add(Parameter(String("x"), "vExpr"));

      return add_defaults( signature );
   }
};

} } } } }

#endif