#include <iostream>
#include <locale>
#include <fstream>
#include <map>
#include <vector>
#include <codecvt>

/**
 * Изначально идея была в том, чтобы считывать и разбивать слова на токены в utf-8, считая что вся
 * пунктуация находится в ASCII. Затем преобразовывать их в UTF-16 через codecvt_utf8_utf16 и предствалять
 * как std::u16string. Но под Linux'ом по крайне мере из коробоки не нашлось подходящей локали
 * для преобразования std::u16string к нижнему регистру. Поэтому решение выглядит сейчас именно так. :)
 *
 * Внимание! Поддерживается только 64-битная версия, 32-битность морально устарела.
 */
using char_t = wchar_t;
using string_t = std::wstring;


/**
 * Фасет с таблицей масок символов, отличающейся от классической тем, что вся пунктуация трактуется
 * как пробельные символы.
 */
struct TreatPunctuationAsWhitespace : public std::ctype<char>
{
    TreatPunctuationAsWhitespace() :
        std::ctype<char>(getTable())
    {

    }

    static const mask* getTable()
    {
        static std::vector<mask> table(classic_table(), classic_table() + table_size);
        for(auto& item : table) {
            // Если символ в исходной таблице имеет маску и это не маска буквы, добавляем маску пробельного символа
            if(item != 0 && !(item & alpha)) {
                item |= space;
            }
        }

        return &table[0];
    }
};


int main(int argc, char** argv) {
    if(argc != 3) {
        std::cerr << "Wrong arguments count" << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream in(argv[1]);
    in.imbue(std::locale(in.getloc(), new TreatPunctuationAsWhitespace()));
    if(!in) {
        std::cerr << "File \"" << argv[1] << "\" not found" << std::endl;
        return EXIT_FAILURE;
    }

    std::wstring_convert<std::codecvt_utf8<char_t>, char_t> convert;
    std::map<string_t, unsigned int> countByWord;

    // Сперва считываем слова просто как массивы байт в utf-8
    std::string word;
    while(in >> word) {
        string_t str = convert.from_bytes(word);
        // Для приведения в нижний регистр используем локаль, соответствующую окружению
        std::use_facet<std::ctype<char_t>>(std::locale("")).tolower(&str[0], &str[0] + str.size());

        // Словом базово считается несколько букв подряд
        if(str.length() > 1) {
            countByWord[str] += 1;
        }
    }

    std::multimap<unsigned int, string_t> wordByCount;
    for(auto iter = countByWord.rbegin(), end = countByWord.rend(); iter != end; ++iter) {
        //TODO: переносить слова move'ом
        // Правильный порядок достигается за счет сохранения multimap порядка вставки, начиная с C++11
        wordByCount.emplace(iter->second, iter->first);
    }

    std::ofstream out(argv[2]);
    for(auto iter = wordByCount.rbegin(), end = wordByCount.rend(); iter != end; ++iter) {
        out << iter->first << " " << convert.to_bytes(iter->second) << std::endl;
    }

    return EXIT_SUCCESS;
}