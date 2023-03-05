# Mysterious inventory manaGement project

## Обьекты

Диаграмма объектов в файле diagram.uxf. Для просмотра и редактирования нужно установить плагин
Visual Code UMLet.

Объекты:

- Symbol Условное обозначение компонента
- Component Компонент (номенклатура)
- Card Карточка учета номенклатуры 
- Component Номенклатура 
- Package Место хранения
- Property особенности исполнения, например, тип корпуса

### Компонент (Номенклатура) Component

Компонент- резистор, конденсатор, микросхема. 

Микросхемы перечисляются по названию. Для пассивных компонент достаточно одного типа.

- Тип (символ) R- резистор, C- емкость, U- микросхема, MC и другие условные обозначения
- Название. Например, "прецизионный резистор" или, для микросхем, ее название- "2SD1825"
- Единица измерения пассивного элемента (для микросхем не используется)
- степень 10 для номинала, записываемой в базе данных. Например, 0- это 10^0 = 1, в ппримере
номинал хранится как целое число в Омах. Для конденсаторов -9 - это 10^-9 = 1/1000000000 

Пример списка компонент:

|Symbol|   Name     | Unit | Pow10 |
|------|------------|------|-------|
| R    | Резистор   | Ом   | 0     |
| С    | Кондесатор | Ф    | -9    |
| U    | 2SD1824    |      |       |
| U    | 2SD1825    |      |       |

В поле Unit для микросхем можно указывать, напримере, условное обозначение назначения

### Карточка учета Card

Из карточки можно узнать, где и сколько компонент есть в остатке.

Для одного компонента может быть заведено несколько карточек учета, отличающихся особенностями исполнения.

Для микросхем в карточке может указываться тип корпуса в списке особенностей и (или) в поле числовое значение- число ножек или другой признак, по которому можно различить тип корпуса.

Для пассивных элементов в карточке в поле числовое значение указывается номинал (для резисторов сопротивление в Омах, для кондесаторов емкость в пикофарадах)

Карточка учета имеет уникальные
- номер
- название

и содержит 

- Номенклатуру (микросхему)
- Список мест хранения (номера коробок)
- Опционально список особенностей исполнения микросхемы, например, тип корпуса
- nominal Числовое значение

На карточку можно ссылаться по ее номеру или уникальному имени.

Примеры имени:

- R100K
- C20мкФ
- 2SD1825
- 2SD1825-dip

Программа (или СУБД) генерирует уникальное имя, соединяя в одну строку:

- Имя компонента
- Номинал (если не пустое), используя частицы для указания порядка степени
- Через знак "-" опциональные особенности, например, тип корпуса

Общее количество подсчитывается суммированием количеств в каждом месте хранения

### Место хранения (коробка, пакет) Package

Место хранения имеет адрес в виде  массив номеров (координат):
- комната или шкаф
- пакет или ящик
- дополнительные номера, если нужно уточнить место хранения

Например, [219, 1]- это железный шкаф в комнате 219, [1, 23]- 23-я коробка Золотовского, [0]-  место неточно

Максимальный размер массива- 4, номер - положительное число от 0 до 65535.

В коробке лежат несколько компонент разных видов.

Коробка имеет номер.

Место хранения указывается в списке в карточке учета, она указывает, сколько компонент одного типа лежит в коробке.

Место хранения компонента содержит

- Карточка (ее номер)
- номер коробки
- количество в коробке
- примечание

Общее количество компонент любого типа может быть подсчитано суммированием количеств в каждом месте хранения каждого компонента.

### Особенности исполнения Property 

Словарь содержит дополнительные свойства, которые могут быть отмечены в карточке:

- тип корпуса микросхемы
- допустимое отклоннеие от номинала (%)
- ...

Словарь может быть произвольно дополнен другими свойствами.

Свойство имеет ключ (typ), используемый для идентификации свойства, и значение.

| typ | value  |
|-----|--------|
| K   | корпус |

## Операции и журнал операций

### Операции

Операции - это изменение в карточке количества в местах хранения следуюзих типов

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

Поиск карточек по названию компонента и номиналу с множителем

- найти_карточки("R", 68, "K")
- найти_карточки("C", 20, "мк")
- найти_карточки("2SD1825") - без номинала

Добавление карточки

- новая_карточка("R", 100, "K", "МЛТ резистор 100K")
- новая_карточка("C", 20, "мк", "20мкФ")
- новая_карточка("2SD1825", "") - без номинала

Добавление в карточку места хранения и количества

- изменить_в_карточке_кол_во_в_месте("=", "R100K", "219-1", "выпаянные", 12)
- изменить_в_карточке_кол_во_в_месте("=", "C20мкФ", "221-1", "старые", 3)
- изменить_в_карточке_кол_во_в_месте("=", "2SD1825", "112-34", "1 этаж", 1)

Первый параметр имеет значения:

- = установить количество равным
- - забрать 
- + добавить

Эти три функции имеют второй вариант, в котором вторым параметром указывается 
не уникальное имя карточки, а уникальный номер карточки

Третий параметр или массив координат (адоес коробки) или строка, в которой координаты разделены нецифровыми символамим
или знаками минус.

Последний параметр задает число добавляемых компонент в место хранения, начиная с 0.

Получение остатка в местах хранения по названию (номиналу)


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

