# База данных микросхем

## Обьекты

Диаграмма объектов находится в файле diagram.uxf. Для просмотра и редактирования нужно установить плагин
Visual Code UMLet.

Объекты:

- Symbol Условное обозначение компонента (R, C, ...). Единица измерения (для пассивных элементов)
- Component Компонент (номенклатура) - Микросхема
- Card Карточка учета номенклатуры - микросхема с учетом исполнения(корпуса) и запасы в разных местах хранения
- Package Место хранения. Место хранения может хранить несколько типов компонент
- Property Описание особенностей исполнения, например, тип корпуса

### Компонент (Номенклатура) Component

Компонент- резистор, конденсатор, микросхема. 

Микросхемы перечисляются по названию. Для пассивных компонент достаточно одного типа. Пассивные компоненты, как правило, имеют одну
основную числовую характеристику - номинал.

- Тип (символ) R- резистор, C- емкость, U- микросхема, MC и другие условные обозначения
- Название. Например, "прецизионный резистор" или, для микросхем, ее название- "2SD1825"
- Единица измерения пассивного элемента (для микросхем не используется, хотя можно указать число ножек, что может указывать на тип корпуса)
- степень 10 для номинала, записываемой в базе данных. Например, 0- это 10^0 = 1, в примере номинал хранится как целое число в Омах. Для конденсаторов -9 - это 10^-9 = 1/1000000000 

Пример списка компонент:

|Symbol|   Name     | Unit | Pow10 |
|------|------------|------|-------|
| R    | Резистор   | Ом   | 0     |
| С    | Кондесатор | Ф    | -9    |
| U    | 2SD1824    |      |       |
| U    | 2SD1825    |      |       |

В поле Unit для микросхем можно указывать, например, условное обозначение назначения (телевизионная, микроконтроллер,..)

### Карточка учета Card

Из карточки можно узнать, где и сколько компонент есть в остатке.

Для одного компонента может быть заведено несколько карточек учета, отличающихся особенностями исполнения (или по местам хранения, как удобно).

Для микросхем в карточке может указываться тип корпуса в списке особенностей и (или) в поле числовое значение- число ножек или другой признак, по которому можно различить тип корпуса.

Для пассивных элементов в карточке в поле числовое значение указывается номинал (для резисторов сопротивление в Омах, для кондесаторов емкость в пикофарадах)

Карточка учета имеет уникальные

- номер
- имя (название)

и содержит 

- Номенклатуру (микросхему)
- Список мест хранения (номера коробок)
- Опционально список особенностей исполнения микросхемы, например, тип корпуса
- nominal Числовое значение

На карточку можно ссылаться по ее номеру или уникальному имени.

Примеры имени:

- 100KОм
- 20мкФ
- 2SD1825
- 2SD1825-dip

Программа (или СУБД) генерирует уникальное имя, соединяя в одну строку:

- Имя компонента
- Номинал (если не пустое), используя частицы для указания порядка степени (кило, мега, милли, микро, пико)
- Через знак "-" опциональные особенности, например, тип корпуса

Номинал отображается в виде трех цифр, слитно с ними частица и единица измерения.

Символ компонента (R, C) с учетом того, что указывается единица измерения, не нужен.

Общее количество подсчитывается суммированием количеств в каждом месте хранения

### Место хранения (коробка, пакет) Package

Место хранения имеет адрес в виде массива номеров (координат):

- комната или шкаф
- пакет или ящик
- коробка и дополнительные номера, если нужно уточнить место хранения

Например, [219, 1]- это железный шкаф в комнате 219, [1, 23]- 23-я коробка Золотовского, [0]-  место неточно

Максимальный размер массива- 4, номер - положительное число от 0 до 65535.

В коробке лежат несколько компонент разных видов.

Комната, шкаф, пакет(ящик), коробка имеют номера.

Место хранения указывается в списке в карточке учета, она указывает, сколько компонент одного типа лежит в коробке.

Место хранения компонента содержит в своей записи

- Карточка (ее номер)
- номер коробки
- количество в коробке

Общее количество компонент любого типа может быть подсчитано суммированием количеств в каждом месте хранения каждого компонента.

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

## Операции и журнал операций

Сотрудники добавляют и забирают детали, при этом в месте хранения записывается остаток деталей.

Остаток не может быть отрицательным.

Действия сотрудников отмечаются в журнале.

### Операции

Операции - это изменение в карточке количества в местах хранения следующих типов

- Пополнение +
- Выдача - 
- Сверка = (при подсчете в коробке уточнен остаток)

Изменения карточек, такие, как 

- Создание карточки
- Удаление карточки

не затрагивает изменения количества в местах хранения и не записывается в журнал

### Журнал операций

В журнале отмечается время операции, кто внес изменение, а также сама карточка и величина изменения

## API

Список карточек

- карточки.список(0, 100)
- карточки.кол-во()

Параметры - начальный индекс, максимальное число в списке

Поиск карточек по названию компонента и номиналу с множителем

- карточки.найти("=", "R", 68, "K")
- карточки.найти("=", "C", 20, "мк")
- карточки.найти("=", "2SD1825") - без номинала

Операции 

- - меньше или равно
- + больше или равно

не реализуются

Поиск карточек в диапазоне

- карточки.найти("R", 68, "K", 100, "K")
- карточки.найти("C", 20, "мк", 100, "мк")

Возвращают список карточек

Добавление, удаление и редактирование карточки

- карточка.изменить("=", "R", 100, "K")
- карточка.изменить("+", "C", 20, "мк")
- карточка.изменить("+", "C", 20, "мк")
- карточка.изменить("-", "2SD1825", 0, "") - без номинала

Первый параметр имеет значения типа операции:

- = установить количество равным
- - забрать 
- + добавить

Удаление карточки приводит к удалению из мест хранения всех записей компонента.

Добавление в карточку места хранения и количества

- карточка.изменить_кол_во_в_месте("=", "R100K", "219-1", 12)
- карточка.изменить_кол_во_в_месте("+", "C20мкФ", "221-1", 3)
- карточка.изменить_кол_во_в_месте("-", "2SD1825", "112-34", 1)

Первый параметр имеет значения типа операции:

- = установить количество равным
- - забрать 
- + добавить

Эти три функции имеют второй вариант, в котором вторым параметром указывается 
не уникальное имя карточки, а уникальный номер карточки

Третий параметр или массив координат (адоес коробки) или строка, в которой координаты разделены нецифровыми символамим
или знаками минус.

Последний параметр задает число добавляемых компонент в место хранения, начиная с 0.

Список мест хранения

- коробка.список(0, 100)

Параметры - начальный индекс, максимальное число в списке

Получение остатка в местах хранения по названию (номиналу) по компонентам

- коробка.список_карточек("219-1")
- коробка.список_карточек("221-1")
- коробка.список_карточек("112-34")

Добавление, удаление и редактирование мест хранения

- коробка.изменить("+", "219-1", "") - добавить пустую
- коробка.изменить("-", "219-1", "") - удалить со всем содержимым
- коробка.изменить("=", "219-1", "219-2") - копирует все содержимое коробки в другую

Третий параметр в двух случаях не используется

## CLI

Список карточек

```
	> card count
	12

	> card list 0 100
	100KОм
	100KОм   10%
	100KОм   МЛТ
	110KОм
	...
```

Параметры - начальный индекс, максимальное число в списке

Поиск карточек по названию компонента и номиналу с множителем

```
	> card 100KОм
	221-1 		1      100KОм   10%
	225-231 	10     100KОм   МЛТ
	125-2 	    20     100KОм
	--
	Total: 31

	> card 120KОм 150KОм
	221-1 		12     110KОм    2%
	225-231 	30     110KОм
	--
	Total: 42
	
	> card 2SD1825
	121-1 		100     2SD1825    DIP
	--
	Total: 100
```

Добавление, удаление и редактирование карточки

```
	> card set 100KОм разброс=10% тип=МЛТ
	> card add 20мкФ тип=электролит
	> card remove 2SD1825 0
```

Первый параметр имеет значения типа операции:

- set, = установить количество равным
- remove, - забрать 
- add, + добавить

Удаление карточки приводит к удалению из мест хранения всех записей компонента.

Добавление в карточку места хранения и количества

```
	> card qty-set 100KОм "219-1" 12
	> card qty-add 100KОм "221-2" 1
	> card qty-remove 2SD1825 "221-2" 1 
```

Первый параметр имеет значения типа операции:

Список мест хранения

```
	> box list 0 100
	  219-1
	  219-2
	  219-3
	  ...
	> box count
	  123
```

Получение остатка в местах хранения по названию (номиналу) по компонентам

```
	> box "219-1"
	  100KОм		100
	  150KОм		200
	  --
	  Total         300
```

Добавление, удаление и редактирование мест хранения

```
	> box add "219-1" - добавить пустую
	> box remove "219-1" - удалить со всем содержимым
	> box set "219-1" "219-2" - копирует все содержимое коробки в другую
```

Добавление, удаление и редактирование свойств

```
	> property list 0 100
	  корпус DIP PDIP
	> property set корпус - очистить
	> property add корпус DIP PDIP CDIP
	> property remove add корпус DIP
```


## GUI

Обозначения

<---> ссылка (кнопка)
[---] поле ввода
[---^] поле ввода с выпадающим списком


### Список карточек с фильтром

```
Имеющиеся компоненты

Фильтр от [ 100КОм ] до [-----]
Особенности исполнения [----^] <Добавить><Удалить> <Применить>

№   Компонент   Коробки
----------------------------------
1   <100КОм>    <219-1 100шт> <221-2 1 шт> <111-23 3шт>
.
.
.
100 <100КОм-МЛТ>    <219-1 100шт> <221-2 1 шт> <111-23 3шт>

Страница 1 из 100 <Пред> <След> 
```
<Добавить компонент>

### Карточка

```
[R^] [100] [К^]  - 100КОм

Особенности исполнения [------^] <Добавить>

Коробки    Штук
----------------------------------
<219-1>    100шт
<221-2>    1шт
<111-23>   3шт
...
<Назад> <Сохранить> <Удалить> 
```

### Коробка

```
100КОм

Коробка [219-1]    [100] шт
...
<Назад> <Сохранить> <Удалить> 
```


## Quick start

Install dependencies and tool first (see section Prerequisites).

Check proto/mgp.proto file.

Determine which classes are persistent in the odb/pragmas.pgsql.hxx.

List classes and members persistent in the relational database in the odb/pragmas.pgsql.hxx file
e.g.

```
	ODB_TABLE(Person)
		ODB_STRING(Person, first_name)
		ODB_STRING(Person, last_name)
      ...
```

Then call ./tools/generate-code.ps1

./tools/generate-code.ps1 do:

- generate serialziation & protocol support classes
- generate object relation mapping (ORM) classes

### Dependencies


tools/install-grpc.sh installs libgrpc++.so.1.24.3


gRPC depends on c-ares
```
sudo apt install libc-ares-dev
```

### Serialzation & protocol support classes

Generate in the gen/ subdirectory protobuf serialization classes  (.pb.cc, .pb.h),
gRPC protocol classes (.grpc.pb.cc, .pb.cc, .pb.h.pb.h) usinbg protoc compiler and 
protoc's grpc c++ plugin.

Object relation mapping (ORM)

Generate in the gen/ subdirectory ODB ORM mapping code(.pb-odb.cxx, .pb-odb.ixx, .pb-odb.hxx),
gRPC protocol classes (.grpc.pb.cc, .pb.cc, .pb.h.pb.h) usinbg protoc compiler and 
protoc's grpc c++ plugin.

by the proto/mgp.proto 

## Tools

- .\tools\generate-code.ps1 - Generate
- .\tools\clean-code.ps1 - Clean

## Prerequisites

### Development tools

- Visual Studio 2022 17.4.5 (Visual C++)
- vcpkg 2021-12-09 (package manager)

### Preinstalled dependencies

Libraries:

- odb (ORM)
- grpc (client-server protocol)
- protobuf (serialization) 

Compilers:

- odb (2.4.0)
- protoc (libprotoc 3.18.0)

must be in the folder in the %PATH%

Copy protoc.exe, grpc_cpp_plugin.exe and *.dll from C:\git\vcpkg\installed\x64-windows-static\tools\protobuf, C:\git\vcpkg\buildtrees\grpc\x64-windows-rel
to C:\bin

Download https://www.codesynthesis.com/download/odb/2.4/odb-2.4.0-i686-windows.zip

Unzip odb-2.4.0-i686-windows.zip to c:\p\odb (for example)

Add C:\p\odb\bin to %PATH%


#### Install odb

Linux:

[Install unix](https://codesynthesis.com/products/odb/doc/install-unix.xhtml)

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

Windows:

```
vcpkg install libodb:x64-windows libodb-pgsql:x64-windows libodb-sqlite:x64-windows
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

#### Install grpc & protobuf

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

##### Windows
```
vcpkg install grpc:x64-windows-static protobuf:x64-windows-static
```

#### Install all

```
vcpkg install libodb:x64-windows-static libodb-pgsql:x64-windows-static libodb-sqlite:x64-windows-static sqlite3:x64-windows-static grpc:x64-windows-static
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

## Generate

Generate files:

- mgp.grpc.pb.cc
- mgp.grpc.pb.h
- mgp.pb-odb.cxx
- mgp.pb-odb.hxx
- mgp.pb-odb.ixx
- mgp.pb.cc
- mgp.pb.h
- mgp.pb.sql

In the PowerShell execute:
```
.\tools\generate-code.ps1
```

If errors occured see Bugs section.

## Clean files

In the PowerShell execute:
```
.\tools\clean-code.ps1
```

## CMake

```
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\git\vcpkg\scripts\buildsystems\vcpkg.cmake
```

## Bugs

### gcc

odb exit with gcc error :

```
C:\git\vcpkg\packages\protobuf_x64-windows-static\include/google/protobuf/stubs/mutex.h:124:29: error: temporary of non-literal type 'google::protobuf::internal::CallOnceInitializedMutex<std::mutex>' in a constant expression
   constexpr WrappedMutex() {}
```

[StackOverflow explanation](https://stackoverflow.com/questions/69232278/c-protocol-buffer-temporary-of-non-literal-type-googleprotobufinternal)


Solution

Remove (comment) line 124 in the C:\git\vcpkg\packages\protobuf_x64-windows-static\include/google/protobuf/stubs/mutex.h


### odb

To see error description, change console code page:
```
chcp 1251
```

