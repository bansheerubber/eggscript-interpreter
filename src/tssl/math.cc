#include "math.h"

// some windows horseshit
#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

#define FMT_HEADER_ONLY
#include "../include/fmt/include/fmt/format.h"
#include <string>

#include "../engine/engine.h"
#include "../util/getEmptyString.h"
#include "../util/stringToChars.h"

using namespace std;

namespace ts {
	namespace sl {
		Entry* mAbs(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				double number = args[0].numberData;
				if(number < 0) {
					return new Entry(-number);
				}
				else {
					return new Entry(number);
				}
			}
			return new Entry(0.0);
		}

		Entry* mACos(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				return new Entry(acos(args[0].numberData));
			}
			return new Entry(0.0);
		}

		Entry* mASin(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				return new Entry(asin(args[0].numberData));
			}
			return new Entry(0.0);
		}

		Entry* mATan(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 2) {
				return new Entry(atan2(args[0].numberData, args[1].numberData));
			}
			return new Entry(0.0);
		}

		Entry* mCeil(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				return new Entry(ceil(args[0].numberData));
			}
			return new Entry(0.0);
		}

		Entry* mFloor(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				double number = args[0].numberData;
				if(number < 0) {
					return new Entry((int)(number - 1));
				}
				else {
					return new Entry((int)number);
				}
			}
			return new Entry(0.0);
		}

		Entry* mCos(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				return new Entry(cos(args[0].numberData));
			}
			return new Entry(0.0);
		}

		Entry* mSin(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				return new Entry(sin(args[0].numberData));
			}
			return new Entry(0.0);
		}

		Entry* mTan(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				return new Entry(tan(args[0].numberData));
			}
			return new Entry(0.0);
		}

		Entry* mPow(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 2) {
				return new Entry(pow(args[0].numberData, args[1].numberData));
			}
			return new Entry(0.0);
		}

		Entry* mSqrt(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				return new Entry(sqrt(args[0].numberData));
			}
			return new Entry(0.0);
		}

		Entry* mDegToRad(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				return new Entry(args[0].numberData * M_PI / 180.0);
			}
			return new Entry(0.0);
		}

		Entry* mRadToDeg(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				return new Entry(args[0].numberData * 180.0 / M_PI);
			}
			return new Entry(0.0);
		}

		Entry* mLog(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				return new Entry(log(args[0].numberData));
			}
			return new Entry(0.0);
		}

		Entry* mFloatLength(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 2) {
				int precision = (int)args[1].numberData;
				if(precision < 0) {
					precision = 0;
				}
				else if(precision > 16) {
					precision = 16;
				}

				string formatter("{:.");
				formatter += std::to_string(precision);
				formatter += "f}";
				string formatted(fmt::format(formatter, args[0].numberData));
				return new Entry(stringToChars(formatted));
			}
			return nullptr;
		}

		Entry* getRandom(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 0) {
				return new Entry(engine->getRandom());
			}
			else if(argc == 1) {
				return new Entry((int)(engine->getRandom() * args[0].numberData));
			}
			else if(argc == 2) {
				return new Entry((int)(engine->getRandom() * (args[1].numberData - args[0].numberData + 1) + args[0].numberData));
			}

			return nullptr;
		}

		Entry* setRandomSeed(Engine* engine, unsigned int argc, Entry* args) {
			if(argc == 1) {
				engine->setRandomSeed((int)args[0].numberData);
			}

			return nullptr;
		}

		Entry* getRandomSeed(Engine* engine, unsigned int argc, Entry* args) {
			return new Entry(engine->getRandomSeed());
		}
	}
}