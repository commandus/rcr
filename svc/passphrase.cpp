/*
 * passphrase.cpp
 *
 *  Created on: Jan 7, 2017
 *      Author: andrei
 *  Inspired by https://github.com/docker/docker/blob/master/pkg/namesgenerator/names-generator.go
 */

#include "passphrase.h"
#include <stdlib.h>
#include <string>
#include <ctime>

const std::string en_left [] = {
	"admiring",
	"adoring",
	"affectionate",
	"agitated",
	"amazing",
	"angry",
	"awesome",
	"blissful",
	"boring",
	"brave",
	"clever",
	"cocky",
	"compassionate",
	"competent",
	"condescending",
	"confident",
	"cranky",
	"dazzling",
	"determined",
	"distracted",
	"dreamy",
	"eager",
	"ecstatic",
	"elastic",
	"elated",
	"elegant",
	"eloquent",
	"epic",
	"fervent",
	"festive",
	"flamboyant",
	"focused",
	"friendly",
	"frosty",
	"gallant",
	"gifted",
	"goofy",
	"gracious",
	"happy",
	"hardcore",
	"heuristic",
	"hopeful",
	"hungry",
	"infallible",
	"inspiring",
	"jolly",
	"jovial",
	"keen",
	"kickass",
	"kind",
	"laughing",
	"loving",
	"lucid",
	"mystifying",
	"modest",
	"musing",
	"naughty",
	"nervous",
	"nifty",
	"nostalgic",
	"objective",
	"optimistic",
	"peaceful",
	"pedantic",
	"pensive",
	"practical",
	"priceless",
	"quirky",
	"quizzical",
	"relaxed",
	"reverent",
	"romantic",
	"sad",
	"serene",
	"sharp",
	"silly",
	"sleepy",
	"stoic",
	"stupefied",
	"suspicious",
	"tender",
	"thirsty",
	"trusting",
	"unruffled",
	"upbeat",
	"vibrant",
	"vigilant",
	"wizardly",
	"wonderful",
	"xenodochial",
	"youthful",
	"zealous",
	"zen"
};

const std::string ru_left [] = {
	"восхитительный",
	"обожающий",
	"любящий",
	"смешивающий",
	"удивительный",
	"сердитый",
	"здоровый",
	"блаженный",
	"скучный",
	"храбрый",
	"умный",
	"дерзкий",
	"сострадательный",
	"компетентный",
	"снисходительный",
	"уверенный в себе",
	"капризный",
	"ослепительный",
	"определенный",
	"отвлекающийся",
	"мечтательный",
	"нетерпеливый",
	"в экстазе",
	"эластичный",
	"приподнятый",
	"элегантный",
	"красноречивый",
	"эпический",
	"пламенный",
	"праздничный",
	"яркий",
	"сосредоточенный",
	"дружелюбный",
	"морозный",
	"галантный",
	"одаренный",
	"бестолковый",
	"милостивый",
	"счастливый",
	"хардкорный",
	"эвристический",
	"надеющийся",
	"голодный",
	"Непогрешимый",
	"вдохновляющий",
	"желейный",
	"бодрый",
	"острый",
	"надирающий задницу",
	"своего рода",
	"смеющийся",
	"любящий",
	"осознанный",
	"мистифицирующий",
	"скромный",
	"размышляющий",
	"непослушный",
	"нервный",
	"изящный",
	"ностальгический",
	"озадаченный",
	"оптимистичный",
	"мирный",
	"педантичный",
	"задумчивый",
	"практичный",
	"бесценный",
	"изворотливый",
	"насмешливый",
	"расслабленный",
	"трепетный",
	"романтический",
	"грустный",
	"спокойный",
	"острый",
	"глупый",
	"сонный",
	"стоический",
	"ошеломленный",
	"подозрительный",
	"нежный",
	"жаждущий",
	"доверчивый",
	"невозмутимый",
	"оптимистичный",
	"яркий",
	"бдительность",
	"волшебный",
	"замечательный",
	"чужеродный",
	"юношеский",
	"ревностный",
	"дзен-буддистский"
};

const std::string en_right [] = {
	"albattani",
	"allen",
	"almeida",
	"agnesi",
	"archimedes",
	"ardinghelli",
	"aryabhata",
	"austin",
	"babbage",
	"banach",
	"bardeen",
	"bartik",
	"bassi",
	"beaver",
	"bell",
	"bhabha",
	"bhaskara",
	"blackwell",
	"bohr",
	"booth",
	"borg",
	"bose",
	"boyd",
	"brahmagupta",
	"brattain",
	"brown",
	"carson",
	"chandrasekhar",
	"shannon",
	"clarke",
	"colden",
	"cori",
	"cray",
	"curran",
	"curie",
	"darwin",
	"davinci",
	"dijkstra",
	"dubinsky",
	"easley",
	"edison",
	"einstein",
	"elion",
	"engelbart",
	"euclid",
	"euler",
	"fermat",
	"fermi",
	"feynman",
	"franklin",
	"galileo",
	"gates",
	"goldberg",
	"goldstine",
	"goldwasser",
	"golick",
	"goodall",
	"haibt",
	"hamilton",
	"hawking",
	"heisenberg",
	"heyrovsky",
	"hodgkin",
	"hoover",
	"hopper",
	"hugle",
	"hypatia",
	"jang",
	"jennings",
	"jepsen",
	"joliot",
	"jones",
	"kalam",
	"kare",
	"keller",
	"khorana",
	"kilby",
	"kirch",
	"knuth",
	"kowalevski",
	"lalande",
	"lamarr",
	"lamport",
	"leakey",
	"leavitt",
	"lewin",
	"lichterman",
	"liskov",
	"lovelace",
	"lumiere",
	"mahavira",
	"mayer",
	"mccarthy",
	"mcclintock",
	"mclean",
	"mcnulty",
	"meitner",
	"meninsky",
	"mestorf",
	"minsky",
	"mirzakhani",
	"morse",
	"murdock",
	"newton",
	"nightingale",
	"nobel",
	"noether",
	"northcutt",
	"noyce",
	"panini",
	"pare",
	"pasteur",
	"payne",
	"perlman",
	"pike",
	"poincare",
	"poitras",
	"ptolemy",
	"raman",
	"ramanujan",
	"ride",
	"montalcini",
	"ritchie",
	"roentgen",
	"rosalind",
	"saha",
	"sammet",
	"shaw",
	"shirley",
	"shockley",
	"sinoussi",
	"snyder",
	"spence",
	"stallman",
	"stonebraker",
	"swanson",
	"swartz",
	"swirles",
	"tesla",
	"thompson",
	"torvalds",
	"turing",
	"varahamihira",
	"visvesvaraya",
	"volhard",
	"wescoff",
	"wiles",
	"williams",
	"wilson",
	"wing",
	"wozniak",
	"wright",
	"yalow",
	"yonath"
};

const std::string ru_right [] = {
	"Аль-Баттани",
	"Аллен",
	"Альмейд",
	"Агнез",
	"Архимед",
	"Ардинелли",
	"Арьябхата",
	"Остин",
	"Бэббидж",
	"Бош",
	"Бард",
	"Барт",
	"Басов",
	"Бобровский",
	"Белл",
	"Бхабха",
	"Бхаскара",
	"Блэквелл",
	"Бор",
	"Будищев",
	"Борг",
	"Боз",
	"Бойд",
	"Брахмагупта",
	"Браттейн",
	"Браун",
	"Карсон",
	"Вернер",
	"Шеннон",
	"Кларк",
	"Голден",
	"Кори",
	"Крей",
	"Киран",
	"Кюри",
	"Дарвин",
	"Да Винчи",
	"Дейкстра",
	"Дубинский",
	"Ираклион",
	"Эдисон",
	"Эйнштейн",
	"Эллиотт",
	"Энгельбарт",
	"Евклид",
	"Эйлер",
	"Ферштейн",
	"Ферми",
	"Фейнман",
	"Франклин",
	"Галилео",
	"Гейтс",
	"Голддберг",
	"Голдштайн",
	"Гольдвасер",
	"Голиков",
	"Гудолл",
	"Цай",
	"Гамильтон",
	"Рейган",
	"Гейзенберг",
	"Гейровский",
	"Линкольн",
	"Вашинтон",
	"Индиана",
	"Джонс",
	"Карпентер",
	"Джан",
	"Дженнингс",
	"Джепсен",
	"Жолио",
	"Джоунс",
	"слон",
	"кот",
	"Келлер",
	"Эпштейн",
	"Килби",
	"Кирха",
	"Кнут",
	"Ковалевский",
	"Лаплас",
	"Ламарр",
	"Лампорт",
	"Лики",
	"Левит",
	"Левин",
	"Лихтерман",
	"Лисков",
	"Лавлейс",
	"Люмьер",
	"махаон",
	"Мейер",
	"МакКарти",
	"МакКлинтон",
	"МакДак",
	"Макналти",
	"Майтнер",
	"Мелинский",
	"Нескафе",
	"Минский",
	"Рональд",
	"Морзе",
	"Мэрдок",
	"Ньютон",
	"Соловей",
	"Нобель",
	"Нётеровский",
	"Норткатт",
	"Нойс",
	"Панини",
	"Пиросмани",
	"Пастер",
	"Payne",
	"Перлман",
	"Свердлов",
	"Пуанкаре",
	"Пойтрас",
	"Птолемей",
	"Рамана",
	"Роман",
	"Монтгомери",
	"Бернс",
	"Ричи",
	"Рентген",
	"Ватт",
	"Гаусс",
	"Хо",
	"Ши",
	"Ширли",
	"Шокли",
	"Синусси",
	"Снайдер",
	"Спенс",
	"Столлман",
	"Стоунбрейкер",
	"Свенсон",
	"Шварц",
	"Копперфильд",
	"Тесла",
	"Томсон",
	"Торвальдс",
	"Туринг",
	"Варахамихира",
	"Форд",
	"Фольксваген",
	"Вермейер",
	"Нельсон",
	"Вильямс",
	"Вилсон",
	"Вингс",
	"Возняк",
	"Райт",
	"Йель",
	"Йонат"
};

void initRandomName()
{
	srand(time(nullptr));
}

// GetRandomName generates a random name from the list of adjectives and surnames in this package
// formatted as "adjective_surname". For example 'focused_turing'. If retry is non-zero, a random
// integer between 0 and 10 will be added to the end of the name, e.g `focused_turing3`

std::string getRandomName()
{
	int left = rand() % (int)(sizeof(ru_left) / sizeof(ru_left[0]));
	int right = rand() % (int)(sizeof(ru_right) / sizeof(ru_right[0]));

	return ru_left[left] + " " + ru_right[right];
}
