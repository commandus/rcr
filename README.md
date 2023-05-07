# База данных микросхем

## Термины

В программе используются сокращенные названия, описанные ниже.

Компонентом называется радиодеталь одного из типов: резистор, конденсатор, индуктивность, микросхема.

Пассивные компоненты, как правило, имеют одну основную числовую характеристику - номинал.

Для пассивных компонент для поиска часто бывает достаточно указать только номинал.

Для микросхем, транзисторов, реле поиск обычно осуществляется по названию детали. 

Название транзистора, например, может выглядеть так: "2SD1825"

Типы компонентов на принципиальных схемах обозначаются символом- одной большой буквой латинского алфавита.

Например, тип резистор имеет символ R, емкость- C, микросхема- D.

Компоненты хранятся в пронумерованных коробках или кассах. Номер коробки- это десятичное число от 1 до 32768.

Коробки хранятся в шкафах, шкафы находятся в комнатах.

Одного номера, чтобы найти коробку, недостаточно. Для указания места хранения используется путь.

Путь- это серия номеров, разделенных знаком "-" (минус).

Например, в комнате 119 находится шкаф или большая коробка с номером 42, а в ней- маленькая коробка из под конфет с номером 13.

Тогда путь будет:

119-42-13

Первый номер- это комната, второй номер- шкаф или большая коробка, третий- место хранения компонента.

Когда нужно найти компонент, можно искать его во всех местах, или ограничить поиск комнатой. Тогда нужно будет указать путь 119.

Если надо ограничить поиск шкафом, нужно указать путь 119-42.

Если ограничения поиска нет, можно указать символ "*".

Карточкой называется запись с названием компонента (и номинала, если есть), ее типом и списком мест хранения.

Список мест хранения состоит из записей. Каждая запись включает в себя путь к коробке и количество в ней этого
компонента.

Из карточки можно узнать, где (в каких коробках), и сколько компонент есть в остатке в каждой коробке.

Для одного компонента может быть заведено несколько карточек учета, отличающихся особенностями исполнения (или по местам хранения, как удобно).

Карточка учета имеет уникальные

- номер (он используется внутри программы)
- имя (название)

и содержит 

- Компонент (например, микросхема D1218)
- Список мест хранения (номера коробок- пути)
- Опционально список особенностей исполнения микросхемы, например, тип корпуса
- nominal Числовое значение номинала для пассивных элементов

На карточку можно ссылаться по ее номеру или уникальному имени.

Для поиска карточек можно использовать или имя компонента, или номинал для пассивных компонентов,
например:

- 100 кОм
- 20мкФ
- 2SD1825
- К155ЛА7 К-dip

Если указывается номинал, то за числом номинала нужно указать единицу измерения с приставкой порядка величины.

Номинал- это целое положительное число.

|Символ| Тип пассивного компонента  | Единица измерения | Допустимые приставки          |
|------|----------------------------|-------------------|-------------------------------|
| R    | Резистор                   | Ом                | к, М, Г, Т, П, Е, З, И, Р, Кв |
| С    | Кондесатор                 | Ф                 | мк, н, п, ф, а, з, и, р, кв   |
| L    | Индуктивность              | Гн                | мк, н, п, ф, а, з, и, р, кв   |

С приставками номинал обычно округляется до трех цифр, для поиска этого достаточно.

Программа (или СУБД) генерирует уникальное имя, соединяя в одну строку:


Все типы компонент:

|Символ| Тип компонента                |
|------|-------------------------------|
|   A  | Устройства                    |
|   B  | Микрофоны, громкоговорители   |
|   C  | Конденсаторы                  |
|   D  | Интегральные схемы            |
|   E  | Разные элементы               |
|   F  | Плавкие предохранители        |
|   G  | Источники питания             |
|   H  | Индикаторы                    |
|   I  | Крепления (*)                 |
|   J  | Провода (*)                   |
|   K  | Реле                          |
|   L  | Дроссели                      |
|   M  | Двигатели                     |
|   N  | На выброс (**)                |
|   O  | Проекты (**)                  |
|   P  | Счетчики                      |
|   Q  | Выключатели                   |
|   R  | Резисторы                     |
|   S  | Переключатели                 |
|   T  | Трансформаторы                |
|   U  | Выпрямители                   |
|   V  | Диоды, тиристоры, транзисторы |
|   W  | Антенны                       |
|   X  | Гнезда                        |
|   Y  | Электромагнитный привод       |
|   Z  | Кварцевые фильтры             |

(*) -  на схемах не обозначается, символ выбран условно
(**) - зарезервировано для перевода компонент в расход в особых случаях

### Место хранения (коробка, пакет) 

Место хранения имеет путь в виде серии номеров, разделяемых символом-разделителем,
номера означают:

- номер комнаты
- номер шкафа
- номер кассы
- номер коробки в кассе

Символ-разделитель- знак минус ("-").

Например, 219-1 это железный шкаф в комнате 219, 119-23- 23-я коробка в кабинете 119.

Максимальный размер массива- 4, номер - положительное число от 1 до 32767 (один бит не используется).

[Замечание 1] Sqlite3 имеет тип Int64, но нет типа Uint64, поэтому в первом числе е пути можно записать 32767, 
но не 65535 

В коробке лежат несколько компонент разных видов.

Комната, шкаф, пакет(ящик), коробка имеют номера.

Место хранения указывается в списке в карточке учета, она указывает, сколько компонент одного типа лежит в коробке.

Место хранения компонента содержит в своей записи

- Карточка (ее номер)
- номер коробки
- количество в коробке

Общее количество компонент любого типа может быть подсчитано суммированием количеств в каждом месте хранения каждого компонента.

### Операции

Операции - это изменение в карточке количества в местах хранения следующих типов

- Пополнение + <число>. Добавляет в коробку заджанное число компонент
- Выдача - <число>. Уменьшает в коробке количество компонент на заданное число
- Присвоение = <число> Устанавливает количество в коробке компонент
- Перемещение / <путь> в другую коробку, заданную путем

Примеры:

Добавить в коробку 119-42-13 два транзистора
```
2SD1825 119-42-13 +2
```

Вынуть из коробки 119-42-13 два транзистора
```
2SD1825 119-42-13 -2
```

Вынуть из всех коробок в комнате 119 по два транзистора из каждой коробки, где она есть:
```
2SD1825 119 -2
```
Если в коробках нет таких транзисторов, ничего не произойдет.

Если в каких-то коробках есть одна микросхема, вынется одна.

Добавить в коробки в комнате 119 по два транзистора в те коробки, где они уже были:
```
2SD1825 119 +2
```
Если в коробках не было таких транзисторов, ничего не произойдет.


Переместить из коробки 119-42-13 два транзистора в коробку 119-42-14
```
2SD1825 119-42-13 /2 119-42-14
```

Переместить из коробки 119-42-13 два транзистора 2SD1825 в коробку 119-42-14
```
2SD1825 119-42-13 / 119-42-14
```

Переместить из коробки 119-42-13 все в коробку 119-42-14
```
* 119-42-13 /2 119-42-14
```

Агрегация

- count - подсчитывеает количество карточек
- sum - подсчитывеает количсество компонентов

Изменения карточек, такие, как 

- Создание карточки
- Удаление карточки

не затрагивает изменения количества в местах хранения и не записывается в журнал

### Журнал операций

В журнале отмечается время операции, кто внес изменение, а также сама карточка и величина изменения

## Утилиты

### Загрузка перечня из файлов книги Excel

Коробки имеют номера.

Коробки вкладываются друг в друга.

Файл книги Excel в названии может иметь несколько числесл с указанием коробки.

Книга Excel может содержать несколько таблиц с произвольными именами.

Имена таблиц в создании иерархии коробок не участвуют.

Колонка "A" файла Excel должна содержать номер коробки.

### Проверка файлов

Проверка

```
xlsx-list "~/src/rcr/data" -b 121
```

Параметр -b хадает номер первой коробки (он может совпадать с номер комнаты, где искать коробку)

#### Загрузка файлов

Например, добавим микросхемы из файлов Excel в каталоге ../data: 

```
./rcr-cli xlsx-add-d ../data -b 219
```

Последняя буква в xlsx-add-X указывает тип компонента
- xlsx-add-A Устройства
- xlsx-add-B Микрофоны, громкоговорители
- xlsx-add-C Конденсаторы
- xlsx-add-D Интегральные схемы
- xlsx-add-E Разные элементы
- xlsx-add-F Плавкие предохранители
- xlsx-add-G Источники питания
- xlsx-add-H Индикаторы
- xlsx-add-K Реле
- xlsx-add-L Дроссели
- xlsx-add-M Двигатели
- xlsx-add-P Счетчики
- xlsx-add-Q Выключатели
- xlsx-add-R Резисторы
- xlsx-add-S Переключатели
- xlsx-add-T Трансформаторы
- xlsx-add-U Выпрямители
- xlsx-add-V Диоды, тиристоры, транзисторы
- xlsx-add-W Антенны
- xlsx-add-X Гнезда
- xlsx-add-Y Электромагнитный привод
- xlsx-add-Z Кварцевые фильтры 

- В каталоге ../data находятся файлы с именами:

```
Мксх_Золотовский_10.xlsx
Мксх_Золотовский_11.xlsx
```

В колонке "A" в каждой электронной таблице указывается номер последей в иерархии в вложенности коробки.

Тогда коробки будут иметь составные номера:

```
121-10-1 - первая коробочка из коробки 10 в кабинете 121 ()
121-10-2 - вторая коробочка из коробки 10 в кабинете 121
...
121-11-1 - первая коробочка из коробки 11 в кабинете 121
121-11-2 - вторая коробочка из коробки 11 в кабинете 121
...
```

## Свойства

Компонент может иметь свойства.

Свойство имеет короткое название, и полное название (описание).

Короткое название обычно сосотоит из одной или двух букв. Регистр букв различается.

### Список свойств

В ../rcr-cli введите
```
property list
```
и получите список свойств, которыми можно охарактеризовать компоненты, например:
```
K	корпус
A	точность
```

### Использование свойств

В ../rcr-cli свойства указываются после названия компонента или его номинала, например, в поиске:
```
100 кОм A:10%
100 кОм A:5% P:1Вт
```

или присвоении числа штук хранения:

```
100 кОм A:10% 119-2 = 7
100 кОм A:5% P:1Вт 119-2 = 6
```

и в других операциях.

Карточки с разными свойствами учитываюься отдельно, то есть, например,
карточка "100 кОм A:10% 119-2" не входит в карточку "100 кОм 119-2".

### Добавление свойства

```
property + A точность
```
 
100 кОм
100 кОм 119-1 = 5
100 кОм :5% K:DIP 119-2 = 6
100 кОм :10% 119-2 = 7


100 кОм
100 кОм 119-1 = 5
100 кОм :5% K:DIP 119-2 = 6
100 кОм :10% 119-2 = 7

### Удаление свойства

```
property - A
```

100 кОм
100 кОм 119-1 = 5
100 кОм A:5% K:DIP 119-2 = 6
100 кОм A:10% 119-2 = 7

### Редактирование свойств

В ../rcr-cli можно изменить полное имя (описание) свлйства:
```
property = A точность
```
Короткое название изменить нельзя (можно сделать в десктоп клиенте).

## Операции и журнал операций

Сотрудники добавляют и забирают детали, при этом в месте хранения записывается остаток деталей.

Остаток не может быть отрицательным.

Действия сотрудников отмечаются в журнале.

Получение остатка в местах хранения по названию (номиналу) по компонентам

## Замечанимя разработчикам

### API

#### Карточки

Удаленный вызов cardQuery() производит большинство манипуляция с карточками.

- user имя пользователя, производящего манипуляцию
- query текст запроса
- measure_symbol тип компонентов, ограничивающего действие запроса
- list смещение (начальный индекс в списке) и максимальное число в списке, имеет значение, если запрос возвращает наблр карточек

В query отправляются такие же запросы, как ./rcr-cli

Результат возвращается в OperationResponse в code (0- операция успешно завершена, не 0- код ошибки) и description- 
краткое описание ошибки.

Если в запросе задан агрегат count или sum, результат возвращается
в OperationResponse в count и sum.

#### Список мест хранения

getBox()

Места хранения обновляются при манипуляциях с карточками.

После вызова cardQuery() список мест хранения может измениться.


### Детали реализации

Первоначальная диаграмма объектов находится в файле diagram.uxf. Для просмотра и редактирования нужно установить плагин
Visual Code UMLet.

Таблицы базы данных:

- Symbol Условное обозначение компонента (R, C, ...). Единица измерения (для пассивных элементов)
- Component Компонент (номенклатура) - Микросхема
- Card Карточка учета номенклатуры - микросхема с учетом исполнения(корпуса) и запасы в разных местах хранения
- Package Место хранения. Место хранения может хранить несколько типов компонент
- Property Описание особенностей исполнения, например, тип корпуса

Пример списка компонент:

| Symbol |   Name     | Unit | Pow10 |
|--------|------------|------|-------|
| R      | Резистор   | Ом   | 0     |
| С      | Кондесатор | Ф    | -9    |
| V      | 2SD1824    |      |       |
| V      | 2SD1825    |      |       |

степень 10 для номинала, записываемой в базе данных. Например, 0- это 10^0 = 1, в примере номинал хранится как целое число в Омах. Для конденсаторов -9 - это 10^-9 = 1/1000000000

В поле Unit для микросхем можно указывать, например, условное обозначение назначения (телевизионная, микроконтроллер,..)

Для микросхем в карточке может указываться тип корпуса в списке особенностей и (или) в поле числовое значение- число ножек или другой признак, по которому можно различить тип корпуса.

Для пассивных элементов в карточке в поле числовое значение указывается номинал (для резисторов сопротивление в Омах, для конденсаторов емкость в пикофарадах)


    COMPONENT_A,    // Устройства
    COMPONENT_B,    // Микрофоны, громкоговорители
    COMPONENT_C,    // Конденсаторы
    COMPONENT_D,    // Интегральные схемы
    COMPONENT_E,    // Разные элементы
    COMPONENT_F,    // Плавкие предохранители
    COMPONENT_G,    // Источники питания
    COMPONENT_H,    // Индикаторы
    COMPONENT_I,    // == Крепления
    COMPONENT_J,    // == Провода
    COMPONENT_K,    // Реле
    COMPONENT_L,    // Дроссели
    COMPONENT_M,    // Двигатели
    COMPONENT_N,    // == На выброс
    COMPONENT_O,    // == Проекты
    COMPONENT_P,    // Счетчики
    COMPONENT_Q,    // Выключатели
    COMPONENT_R,    // Резисторы
    COMPONENT_S,    // Переключатели
    COMPONENT_T,    // Трансформаторы
    COMPONENT_U,    // Выпрямители
    COMPONENT_V,    // Диоды, тиристоры, транзисторы
    COMPONENT_W,    // Антенны
    COMPONENT_X,    // Гнезда
    COMPONENT_Y,    // Электромагнитный привод
    COMPONENT_Z     // Кварцевые фильтры



- Имя компонента
- Номинал (если не пустое), используя частицы для указания порядка степени (кило, мега, милли, микро, пико)
- Через знак "-" опциональные особенности, например, тип корпуса

Номинал отображается в виде трех цифр, слитно с ними частица и единица измерения.

Символ компонента (R, C) с учетом того, что указывается единица измерения, не нужен.

Общее количество подсчитывается суммированием количеств в каждом месте хранения

### Особенности исполнения Property

Словарь Property содержит дополнительные свойства, которые могут быть отмечены в карточке:

- тип корпуса микросхемы
- допустимое отклонеие от номинала (%)
- ...

Словарь может быть произвольно дополнен другими свойствами.

Свойство имеет ключ (typ), используемый для идентификации свойства, и значение.

| typ | value  |
|-----|--------|
| D   | корпус |


## Инструменты

- CMake
- Visual Studio 2022 17.4.5 (Visual C++) (Windows
- vcpkg 2021-12-09 (Windows)
- Gettext
- компилятор odb (2.4.0)
- компилятор protoc (libprotoc 3.18.0)

Для Windows скачайте https://www.codesynthesis.com/download/odb/2.4/odb-2.4.0-i686-windows.zip

Распакуйте odb-2.4.0-i686-windows.zip в корень, нужны папки
- C:\bin
- C:\etc
- C:\mingw

Папку C:\bin включить в переменную окружения PATH.

Для Windows скопируйте 

- protoc.exe
- grpc_cpp_plugin.exe
- C:\git\vcpkg\installed\x64-windows-static\tools\protobuf\*.dll
- C:\git\vcpkg\buildtrees\grpc\x64-windows-rel\*.dll 

в указываемый переменнгой окружения PATH путь, например, C:\bin

В скрипте tools/generate-code.ps1 в строке 2 исправьте путь к плагину
```
$GRPC_PLUGIN = "c:\p\bin\grpc_cpp_plugin.exe"
```
Плагин указзывается по полному пути, в переменной PATH плагин grpc компилятра protoc
не ищет. В linux скрипте используется `which` для полстановки полного пути .

### Установка vcpkg в Windows:

```
cd \git
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
```

## Зависимости

- ICU / Intl
- Xlnt
- Protobuf
- gRPC
- unofficial-sqlite3/SQLite3(Windows)
- or unofficial-postgresql/PostgreSQL(Windows)
- Xlnt - есть в vcpkg для Windows, но нет в пакетах Ubuntu

Установка в Linux зависимостей из пакетов:

```
sudo apt install grpc-proto libgrpc++-dev libgrpc-dev protobuf-compiler-grpc protobuf-compiler libprotobuf-dev \
libc-ares-dev odb libodb-sqlite-2.4 libicu-dev
```

[Install ODB unix](https://codesynthesis.com/products/odb/doc/install-unix.xhtml)

```
sudo apt install odb libodb-pgsql-2.4
```

Check odb version

```
odb --version
ODB object-relational mapping (ORM) compiler for C++ 2.4.0
```

If it prints error

```
g++-10: error: No such file or directory
```

then install g++-10

```
sudo apt install g++-10
```

### Xlnt

В Linux Xlnt нужно собрать вручную:

```
git clone https://github.com/tfussell/xlnt.git
cd xmake lnt
git submodule init
git submodule update
mkdir build
cd build
cmake ..
make
sudo make install
ls /usr/local/lib/libxlnt.so.1.5.0
ls /usr/local/include/xlnt/xlnt.hpp
```

### Установка зависимостей в Windows

```
cd \git\vcpkg
vcpkg install gettext[tools]:x64-windows libodb:x64-windows libodb-sqlite:x64-windows protobuf:x64-windows grpc:x64-windows icu:x64-windows xlnt:x64-windows
```

Static 
```
cd \git\vcpkg
vcpkg install libodb:x64-windows-static libodb-pgsql:x64-windows-static libodb-sqlite:x64-windows-static sqlite3:x64-windows-static grpc:x64-windows-static protobuf:x64-windows-static
```

###№ Установка odb

```
cd \git\vcpkg
vcpkg install libodb:x64-windows libodb-pgsql:x64-windows libodb-sqlite:x64-windows protobuf:x64-windows grpc:x64-windows
cd \src\rcr
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\git\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
```

```
vcpkg install libodb:x64-windows-static
```

https://www.codesynthesis.com/download/odb/2.4/libodb-2.4.0.zip

PostgreSQL backend

- libodb-pgsql (with libpq)

Install:

```
vcpkg install libodb-pgsql:x64-windows-static
```

SQLite backend

- libodb-sqlite

```
vcpkg install libodb-sqlite:x64-windows-static
```

or download from https://www.codesynthesis.com/download/odb/2.4/libodb-pgsql-2.4.0.zip
upgrade solution from 10 to 14 and build release


### Альтернативные способы установки зависимостей 

[Build and install gRPC and Protocol Buffers instructions](https://grpc.io/docs/languages/cpp/quickstart/)

tools/install-grpc.sh script installs libgrpc++.so.1.24.3

libprotoc 3.12.4
```
sudo apt install --reinstall grpc-proto libgrpc++-dev libgrpc-dev protobuf-compiler-grpc protobuf-compiler libprotobuf-dev
```

```
git clone --recurse-submodules -b v1.24.3 --depth 1 --shallow-submodules https://github.com/grpc/grpc
cd grpc
mkdir -p cmake/build
pushd cmake/build
cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_SSL_PROVIDER:STRING=package ../..
make -j 1
sudo make install
popd
```

gRPC depends on c-ares
```
sudo apt install libc-ares-dev
```

## Создание решения в Windows

Для создания решения Visual Studio воспользуйтесь CMake:

```
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\git\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
или
cmake -A x64 -D CMAKE_TOOLCHAIN_FILE=c:/git/vcpkg/scripts/buildsystems/vcpkg.cmake -D VCPKG_TARGET_TRIPLET=x64-windows ..
```

Check proto/rcr.proto file.

Determine which classes are persistent in the odb/pragmas.pgsql.hxx.

List classes and members persistent in the relational database in the odb/pragmas.pgsql.hxx file
e.g.

```
	ODB_TABLE(Card)
		ODB_STRING(Card, name)
      ...
```

Then call ./tools/generate-code.ps1

./tools/generate-code.ps1 do:

- generate serialziation & protocol support classes
- generate object relation mapping (ORM) classes

### Serialzation & protocol support classes

Generate in the gen/ subdirectory protobuf serialization classes  (.pb.cc, .pb.h),
gRPC protocol classes (.grpc.pb.cc, .pb.cc, .pb.h.pb.h) usinbg protoc compiler and 
protoc's grpc c++ plugin.

Object relation mapping (ORM)

Generate in the gen/ subdirectory ODB ORM mapping code(.pb-odb.cxx, .pb-odb.ixx, .pb-odb.hxx),
gRPC protocol classes (.grpc.pb.cc, .pb.cc, .pb.h.pb.h) usinbg protoc compiler and 
protoc's grpc c++ plugin.

by the proto/rcr.proto 

## Tools

- .\tools\generate-code.ps1 - Generate
- .\tools\clean-code.ps1 - Clean


##### Linux

Check grpc_cpp_plugin is installed
```
which grpc_cpp_plugin
```

If not, add grpc_cpp_plugin to the $PATH

```
cd ~/src-old/third_party/grpc/bins/opt
sudo cp grpc_cpp_plugin  grpc_node_plugin  grpc_objective_c_plugin  grpc_php_plugin  grpc_python_plugin  grpc_ruby_plugin /usr/local/bin
```


find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(main PRIVATE unofficial::sqlite3::sqlite3)

find_package(odb CONFIG REQUIRED)
target_link_libraries(main PRIVATE odb::libodb)

find_package(odb CONFIG REQUIRED)
target_link_libraries(main PRIVATE odb::libodb-pgsql)

find_package(odb CONFIG REQUIRED)
target_link_libraries(main PRIVATE odb::libodb-sqlite)

find_package(gRPC CONFIG REQUIRED)
target_link_libraries(main PRIVATE gRPC::gpr gRPC::upb gRPC::grpc gRPC::grpc++)

find_package(modules CONFIG REQUIRED)
target_link_libraries(main PRIVATE re2::re2 c-ares::cares)

find_package(protobuf CONFIG REQUIRED)
target_link_libraries(main PRIVATE protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)


C:\git\vcpkg\installed\x64-windows-static\tools\protobuf\protoc
C:\git\vcpkg\installed\x64-windows-static\lib\ C:\git\vcpkg\installed\x64-windows-static\lib\ 

#### Generate

Generate files:

- rcr.grpc.pb.cc
- rcr.grpc.pb.h
- rcr.pb-odb.cxx
- rcr.pb-odb.hxx
- rcr.pb-odb.ixx
- rcr.pb.cc
- rcr.pb.h
- rcr.pb.sql

In the PowerShell execute:
```
.\tools\generate-code.ps1
```

If errors occured see Bugs section.

#### Clean files

In the PowerShell execute:
```
.\tools\clean-code.ps1
```

#### CMake

```
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\git\vcpkg\scripts\buildsystems\vcpkg.cmake
```

#### Bugs

##### Windows script generate-code.ps1 policy issue

Generating C:/git/rcr/gen/rcr.grpc.pb.cc, C:/git/rcr/gen/rcr.pb.cc, C:/git/rcr/gen/rcr.pb-odb.cxx, C:/git/rcr/gen/rcr.grpc.pb.h, C:/git/rcr/gen/rcr.pb.h, C:/git/rcr/gen/rcr.pb-odb.hxx
C:/git/rcr/tools/generate-code.ps1 : File C:\git\rcr\tools\generate-code.ps1 cannot be loaded. The file C:\git\rcr\tools\generate-code.ps1 is not digitally signed. You cannot run this script on the current system. For more information about running scripts and setting execution policy, see about_Execution_Policies at https:/go.microsoft.com/fwlink/?LinkID=135170.

PowerShell run as administrator:
```
Set-ExecutionPolicy -ExecutionPolicy Bypass
```
##### gcc

tools/generate-code.ps1 on odb calls gcc

odb exit with gcc error :

```
C:\git\vcpkg\packages\protobuf_x64-windows-static\include/google/protobuf/stubs/mutex.h:124:29: error: temporary of non-literal type 'google::protobuf::internal::CallOnceInitializedMutex<std::mutex>' in a constant expression
   constexpr WrappedMutex() {}
```

[StackOverflow explanation](https://stackoverflow.com/questions/69232278/c-protocol-buffer-temporary-of-non-literal-type-googleprotobufinternal)


Solution

Remove (comment) line 124 in the C:\git\vcpkg\packages\protobuf_x64-windows-static\include/google/protobuf/stubs/mutex.h
(C:\git\vcpkg\packages\protobuf_x64-windows\include\google\protobuf\stubs\mutex.h)

```
#if defined(__QNX__)
  constexpr WrappedMutex() = default;
#else
// constexpr <-- comment this!
  WrappedMutex() {}
#endif
```
#### Visual Studio C++17 deprecated issue

On error 'ptr_fun': is not a member of visual studio
see https://stackoverflow.com/questions/48882439/how-to-restore-auto-ptr-in-visual-studio-c17

1. Project > Properties > C/C++ > Preprocessor > Preprocessor Definitions and add _HAS_AUTO_PTR_ETC=1. Do so for all configurations and platforms.
2. If you use a precompiled header then you probably favor defining the macro there. Before any #includes, insert #define _HAS_AUTO_PTR_ETC 1.


#### odb

To see error description, change console code page:
```
chcp 1251
```

#### protoc

Windows issue:

```
gen/rcr.pb.h:10:40: fatal error: google/protobuf/port_def.inc: No such file or directory
#include <google/protobuf/port_def.inc>
```

Set valid path in tools/generate-code.ps1

```
$PROTOBUF_INC = "C:\git\vcpkg\packages\protobuf_x64-windows-static\include"
```

#### Python client

```
sudo python3 -m pip install grpcio
python3 -m pip install grpcio
python3 -m pip install grpcio-tools
```

##### Generate python code from the proto file

```
cd proto
python3 -m grpc_tools.protoc -I. --python_out=. --pyi_out=. --grpc_python_out=. rcr.proto
cp
``` 

##### Run an example

```
cd python
python3 rcr_client.py
cp
``` 


### Локализация

После правки po/rcr-cli.ru_RU.UTF-8.po запустите
```
tools/update-translation.sh
```
Созданный файл скопируйте в /usr/share/locale/ru/LC_MESSAGES/: 

```
msgfmt -o locale/ru/LC_MESSAGES/rcr-cli.mo po/rcr-cli.ru_RU.UTF-8.po
sudo cp locale/ru/LC_MESSAGES/rcr-cli.mo /usr/share/locale/ru/LC_MESSAGES/rcr-cli.mo
sudo cp locale/ru/LC_MESSAGES/rcr-cli.mo /usr/local/share/locale/ru/LC_MESSAGES/rcr-cli.mo
```

Скрипт tools/l10n создает начальный файл
