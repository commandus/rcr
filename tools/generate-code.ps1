# Attention! Check this hardcoded path!
$GRPC_PLUGIN = "c:\bin\grpc_cpp_plugin.exe"
# -------------------------- Do not edit below this line --------------------------
$WORK_DIR = $($args[0])
$ODB_DATABASE_NAME = $($args[1])
# $PROTOBUF_INC = "C:\git\vcpkg\packages\protobuf_x64-windows\include"
$PROTOBUF_INC = "C:\git\vcpkg\installed\x64-windows\include"
$DEST = "."
$GEN = "gen"
$ODB_OPT_DIR = "odb"
$DT = Get-Date -F "yyyy-MM-dd hh:mm"
$GEN_HDR_PB = "$GEN/rcr.pb.h"
$HVIEWS = "odb/odb-views.h"

$GEN_HDR_GRPC = "GEN/rcr.grpc.pb.h"
$GEN_HDR = "$GEN_HDR_PB $GEN_HDR_GRPC"
$GEN_SRC="$GEN/rcr.pb.cc $GEN/rcr.grpc.pb.cc"
$GEN_FILES="$GEN_HDR $GEN_SRC"
$PROTO_DIR = "proto"
$PROTO = "$PROTO_DIR/rcr.proto"
$H = "$GEN/rcr.pb.h"

if ($null -ne $WORK_DIR) {
    cd $WORK_DIR
}

if ($null -eq $ODB_DATABASE_NAME) {
    $ODB_DATABASE_NAME = "sqlite"
}

function Add-Proto {
    param (
        $OutputDir, $IncludeDir, $ProtoFile, $PluginFile
    )
    # Seriaization
    protoc -I $IncludeDir --cpp_out=$OutputDir $ProtoFile
    # gRPC
    protoc -I $IncludeDir -I $PROTOBUF_INC --grpc_out=$OutputDir --plugin=protoc-gen-grpc=$PluginFile $ProtoFile
}

$PRECLASS = " private:`r`n  friend class odb::access;`r`n  // @@prepare-pb-odb-class(`$1.`$2)"
$PRIV = "public:
  const std::vector<\1::`$(ACCESSVECTORCLASS)*> m`$SB;
  std::vector<\1::`$(ACCESSVECTORCLASS)*>& get`$SB() const
  {
	  std::vector<\1::`$(ACCESSVECTORCLASS)*> *r = (std::vector<\1::`$(ACCESSVECTORCLASS)*>*) &m`$SB;
	  r->clear();
	  const \1::`$(ACCESSVECTORCLASS)* const* d = `$SB_.data();
	  for (int i = 0; i < r->size(); i++)
	  {
		  r->push_back((\1::`$(ACCESSVECTORCLASS)*) *d);
		  d++;
	  }
	return *r;
  };

  void set`$SB(std::vector<\1::`$(ACCESSVECTORCLASS)*> v)
  {
	  `$SB_.Clear();
	  for (std::vector<`$1::`$(ACCESSVECTORCLASS)*>::iterator it = v.begin(); it != v.end(); ++it)
	  {
		  \1::`$(ACCESSVECTORCLASS)* h = `$SB_.Add();
		  h = *it;
	  }
  };
  private:`r`n";

function Add-Vector {
    param (
        $OwnerClass, $MemberClass, $MemberName, $NameSpace
    )
    # insert template
    (Get-Content $GEN_HDR_PB) -Replace "\ \ \/\/ @@prepare-pb-odb-class\((.*)\.($OwnerClass)\)", "  // @@prepare-pb-odb-class(`$1.`$2)`r`n$PRIV" | Set-Content $GEN_HDR_PB
    # substitute member name
    (Get-Content $GEN_HDR_PB) -Replace "\$\SB", "$MemberName" | Set-Content $GEN_HDR_PB
    # substitute name space
    (Get-Content $GEN_HDR_PB) -Replace "\\1", "$NameSpace" | Set-Content $GEN_HDR_PB
    (Get-Content $GEN_HDR_PB) -Replace "\$\(ACCESSVECTORCLASS\)", "$MemberClass" | Set-Content $GEN_HDR_PB
}

# Add ODB pragmas into generated protobuf classes
function Add-Odb {
    param (
        $HeadefFile,
        $AdditionalComment
    )
    # Add <odb/core.hxx> header
    (Get-Content $HeadefFile) -Replace "@@protoc_insertion_point\((includes)\)",
        "@@protoc_insertion_point(`$1)`r`n//-- autogenerated by tools/generate-code.ps1. $AdditionalComment --`r`n#include <odb/core.hxx>" | Set-Content $HeadefFile
    # Add friend class odb::access to each class and comment it by // @@prepare-pb-odb-class(class)
    (Get-Content $HeadefFile) -Replace "\ \ \/\/ @@protoc_insertion_point\(class_scope:(.*)\.(.*)\)",
        "  // @@protoc_insertion_point(--class_scope:`$1.`$2)`r`n$PRECLASS" | Set-Content $HeadefFile
}

function Odb-Db {
    param (
        $Driver, $OutDir, $SetDef, $HeaderFile
    )
    Write-Output("odb -o $OutDir -d $Driver -x -std=c++14 --options-file odb\options.pgsql --fkeys-deferrable-mode not_deferrable --generate-query --generate-schema -I . -I $ODB_OPT_DIR -I $PROTOBUF_INC $SetDef $HeaderFile")
    odb -o $OutDir -d $Driver -x -std=c++14 --options-file odb\options.pgsql --fkeys-deferrable-mode not_deferrable --generate-query --generate-schema -I . -I $ODB_OPT_DIR -I $PROTOBUF_INC $SetDef $HeaderFile
}

New-Item -ItemType Directory -Force -Path $GEN | Out-Null

# generate protobuf and gRPC
Add-Proto -OutputDir $GEN -IncludeDir $PROTO_DIR -ProtoFile $PROTO -PluginFile $GRPC_PLUGIN
Add-Odb -HeadefFile $GEN_HDR_PB -AdditionalComment $DT

# repeated members like Add-Vector -OwnerClass Person -MemberClass MediaFile -MemberName medias
# Add-Vector -OwnerClass Person -MemberClass MediaFile -MemberName medias -NameSpace rcr
# Add-Vector -OwnerClass Employee -MemberClass MediaFile -MemberName medias -NameSpace rcr
# Add-Vector -OwnerClass Org -MemberClass MediaFile -MemberName medias -NameSpace rcr

# generate ODB ORM
Odb-Db -Driver $ODB_DATABASE_NAME -OutDir $GEN -SetDef "-DGEN_ODB" -HeaderFile $GEN_HDR_PB
Odb-Db -Driver $ODB_DATABASE_NAME -OutDir $GEN -SetDef "-DGEN_ODB" -HeaderFile $HVIEWS

# sed -i "s//" $F
# SET ODBINC="-I/usr/include/x86_64-linux-gnu -I/usr/local/include -I%DEST%/odb"
# SET ODBDEFS="-DGEN_ODB -D__x86_64__ -D__LP64__"
# odb -o $DEST -d pgsql -x '-w' --fkeys-deferrable-mode not_deferrable --generate-query --generate-schema --options-file %DEST%/odb/options.pgsql %ODBINC% %ODBDEFS% %H%
# odb -o $DEST -d pgsql -x '-w' --fkeys-deferrable-mode not_deferrable --generate-query %ODBINC% %ODBDEFS% %HVIEWS%