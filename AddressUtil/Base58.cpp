 (cd "$(git rev-parse --show-toplevel)" && git apply --3way <<'EOF' 
diff --git a/AddressUtil/Base58.cpp b/AddressUtil/Base58.cpp
index 2d80ef3509d4544f97b3e4becaf30be971573dee..e1728809b59252ac763744211ceb307c61f1fcc1 100644
--- a/AddressUtil/Base58.cpp
+++ b/AddressUtil/Base58.cpp
@@ -1,117 +1,124 @@
-#include <map>
+#include <array>
 #include "CryptoUtil.h"
 
 #include "AddressUtil.h"
 
 
 static const std::string BASE58_STRING = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
 
 struct Base58Map {
-	static std::map<char, int> createBase58Map()
+	static std::array<int, 128> createBase58Map()
 	{
-		std::map<char, int> m;
+		std::array<int, 128> m{};
+		m.fill(-1);
 		for(int i = 0; i < 58; i++) {
-			m[BASE58_STRING[i]] = i;
+			m[(unsigned char)BASE58_STRING[i]] = i;
 		}
 
 		return m;
 	}
 
-	static std::map<char, int> myMap;
+	static std::array<int, 128> myMap;
 };
 
-std::map<char, int> Base58Map::myMap = Base58Map::createBase58Map();
+std::array<int, 128> Base58Map::myMap = Base58Map::createBase58Map();
 
 
 
 /**
  * Converts a base58 string to uint256
  */
 secp256k1::uint256 Base58::toBigInt(const std::string &s)
 {
 	secp256k1::uint256 value;
 
 	for(unsigned int i = 0; i < s.length(); i++) {
 		value = value.mul(58);
 
-		int c = Base58Map::myMap[s[i]];
-		value = value.add(c);
+		const unsigned char c = (unsigned char)s[i];
+		if(c >= Base58Map::myMap.size() || Base58Map::myMap[c] < 0) {
+			throw std::string("Invalid base58 character");
+		}
+		int valueChar = Base58Map::myMap[c];
+		value = value.add(valueChar);
 	}
 
 	return value;
 }
 
 void Base58::toHash160(const std::string &s, unsigned int hash[5])
 {
 	secp256k1::uint256 value = toBigInt(s);
 	unsigned int words[6];
 
 	value.exportWords(words, 6, secp256k1::uint256::BigEndian);
 
 	// Extract words, ignore checksum
 	for(int i = 0; i < 5; i++) {
 		hash[i] = words[i];
 	}
 }
 
-bool Base58::isBase58(std::string s)
+bool Base58::isBase58(const std::string &s)
 {
 	for(unsigned int i = 0; i < s.length(); i++) {
-		if(BASE58_STRING.find(s[i]) < 0) {
+		const unsigned char c = (unsigned char)s[i];
+		if(c >= Base58Map::myMap.size() || Base58Map::myMap[c] < 0) {
 			return false;
 		}
 	}
 
 	return true;
 }
 
 std::string Base58::toBase58(const secp256k1::uint256 &x)
 {
 	std::string s;
+	s.reserve(50);
 
 	secp256k1::uint256 value = x;
 
 	while(!value.isZero()) {
 		secp256k1::uint256 digit = value.mod(58);
 		int digitInt = digit.toInt32();
 
 		s = BASE58_STRING[digitInt] + s;
 
 		value = value.div(58);
 	}
 
 	return s;
 }
 
 void Base58::getMinMaxFromPrefix(const std::string &prefix, secp256k1::uint256 &minValueOut, secp256k1::uint256 &maxValueOut)
 {
 	secp256k1::uint256 minValue = toBigInt(prefix);
 	secp256k1::uint256 maxValue = minValue;
 	int exponent = 1;
 
 	// 2^192
 	unsigned int expWords[] = { 0, 0, 0, 0, 0, 0, 1, 0 };
 
 	secp256k1::uint256 exp(expWords);
 
 	// Find the smallest 192-bit number that starts with the prefix. That is, the prefix multiplied
 	// by some power of 58
 	secp256k1::uint256 nextValue = minValue.mul(58);
 
 	while(nextValue.cmp(exp) < 0) {
 		exponent++;
 		minValue = nextValue;
 		nextValue = nextValue.mul(58);
 	}
 
 	secp256k1::uint256 diff = secp256k1::uint256(58).pow(exponent - 1).sub(1);
 
 	maxValue = minValue.add(diff);
 
 	if(maxValue.cmp(exp) > 0) {
 		maxValue = exp.sub(1);
 	}
 
 	minValueOut = minValue;
 	maxValueOut = maxValue;
-}
\ No newline at end of file
+}
 
EOF
)
