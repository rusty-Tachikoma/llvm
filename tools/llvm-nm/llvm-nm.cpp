//===-- llvm-nm.cpp - Symbol table dumping utility for llvm ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This program is a utility that works like traditional Unix "nm", that is, it
// prints out the names of symbols in a bitcode or object file, along with some
// information about each symbol.
//
// This "nm" supports many of the features of GNU "nm", including its different
// output formats.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Object/Archive.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/IRObjectFile.h"
#include "llvm/Object/MachO.h"
#include "llvm/Object/MachOUniversal.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/COFF.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <system_error>
#include <vector>
using namespace llvm;
using namespace object;

namespace {
enum OutputFormatTy { bsd, sysv, posix, darwin };
cl::opt<OutputFormatTy> OutputFormat(
    "format", cl::desc("Specify output format"),
    cl::values(clEnumVal(bsd, "BSD format"), clEnumVal(sysv, "System V format"),
               clEnumVal(posix, "POSIX.2 format"),
               clEnumVal(darwin, "Darwin -m format"), clEnumValEnd),
    cl::init(bsd));
cl::alias OutputFormat2("f", cl::desc("Alias for --format"),
                        cl::aliasopt(OutputFormat));

cl::list<std::string> InputFilenames(cl::Positional, cl::desc("<input files>"),
                                     cl::ZeroOrMore);

cl::opt<bool> UndefinedOnly("undefined-only",
                            cl::desc("Show only undefined symbols"));
cl::alias UndefinedOnly2("u", cl::desc("Alias for --undefined-only"),
                         cl::aliasopt(UndefinedOnly));

cl::opt<bool> DynamicSyms("dynamic",
                          cl::desc("Display the dynamic symbols instead "
                                   "of normal symbols."));
cl::alias DynamicSyms2("D", cl::desc("Alias for --dynamic"),
                       cl::aliasopt(DynamicSyms));

cl::opt<bool> DefinedOnly("defined-only",
                          cl::desc("Show only defined symbols"));
cl::alias DefinedOnly2("U", cl::desc("Alias for --defined-only"),
                       cl::aliasopt(DefinedOnly));

cl::opt<bool> ExternalOnly("extern-only",
                           cl::desc("Show only external symbols"));
cl::alias ExternalOnly2("g", cl::desc("Alias for --extern-only"),
                        cl::aliasopt(ExternalOnly));

cl::opt<bool> BSDFormat("B", cl::desc("Alias for --format=bsd"));
cl::opt<bool> POSIXFormat("P", cl::desc("Alias for --format=posix"));
cl::opt<bool> DarwinFormat("m", cl::desc("Alias for --format=darwin"));

static cl::list<std::string>
    ArchFlags("arch", cl::desc("architecture(s) from a Mach-O file to dump"),
              cl::ZeroOrMore);
bool ArchAll = false;

cl::opt<bool> PrintFileName(
    "print-file-name",
    cl::desc("Precede each symbol with the object file it came from"));

cl::alias PrintFileNameA("A", cl::desc("Alias for --print-file-name"),
                         cl::aliasopt(PrintFileName));
cl::alias PrintFileNameo("o", cl::desc("Alias for --print-file-name"),
                         cl::aliasopt(PrintFileName));

cl::opt<bool> DebugSyms("debug-syms",
                        cl::desc("Show all symbols, even debugger only"));
cl::alias DebugSymsa("a", cl::desc("Alias for --debug-syms"),
                     cl::aliasopt(DebugSyms));

cl::opt<bool> NumericSort("numeric-sort", cl::desc("Sort symbols by address"));
cl::alias NumericSortn("n", cl::desc("Alias for --numeric-sort"),
                       cl::aliasopt(NumericSort));
cl::alias NumericSortv("v", cl::desc("Alias for --numeric-sort"),
                       cl::aliasopt(NumericSort));

cl::opt<bool> NoSort("no-sort", cl::desc("Show symbols in order encountered"));
cl::alias NoSortp("p", cl::desc("Alias for --no-sort"), cl::aliasopt(NoSort));

cl::opt<bool> ReverseSort("reverse-sort", cl::desc("Sort in reverse order"));
cl::alias ReverseSortr("r", cl::desc("Alias for --reverse-sort"),
                       cl::aliasopt(ReverseSort));

cl::opt<bool> PrintSize("print-size",
                        cl::desc("Show symbol size instead of address"));
cl::alias PrintSizeS("S", cl::desc("Alias for --print-size"),
                     cl::aliasopt(PrintSize));

cl::opt<bool> SizeSort("size-sort", cl::desc("Sort symbols by size"));

cl::opt<bool> WithoutAliases("without-aliases", cl::Hidden,
                             cl::desc("Exclude aliases from output"));

cl::opt<bool> ArchiveMap("print-armap", cl::desc("Print the archive map"));
cl::alias ArchiveMaps("M", cl::desc("Alias for --print-armap"),
                      cl::aliasopt(ArchiveMap));

cl::opt<bool> JustSymbolName("just-symbol-name",
                             cl::desc("Print just the symbol's name"));
cl::alias JustSymbolNames("j", cl::desc("Alias for --just-symbol-name"),
                          cl::aliasopt(JustSymbolName));

// FIXME: This option takes exactly two strings and should be allowed anywhere
// on the command line.  Such that "llvm-nm -s __TEXT __text foo.o" would work.
// But that does not as the CommandLine Library does not have a way to make
// this work.  For now the "-s __TEXT __text" has to be last on the command
// line.
cl::list<std::string> SegSect("s", cl::Positional, cl::ZeroOrMore,
                              cl::desc("Dump only symbols from this segment "
                                       "and section name, Mach-O only"));

cl::opt<bool> FormatMachOasHex("x", cl::desc("Print symbol entry in hex, "
                                             "Mach-O only"));

bool PrintAddress = true;

bool MultipleFiles = false;

bool HadError = false;

std::string ToolName;
}

static void error(Twine Message, Twine Path = Twine()) {
  HadError = true;
  errs() << ToolName << ": " << Path << ": " << Message << ".\n";
}

static bool error(std::error_code EC, Twine Path = Twine()) {
  if (EC) {
    error(EC.message(), Path);
    return true;
  }
  return false;
}

namespace {
struct NMSymbol {
  uint64_t Address;
  uint64_t Size;
  char TypeChar;
  StringRef Name;
  DataRefImpl Symb;
};
}

static bool compareSymbolAddress(const NMSymbol &A, const NMSymbol &B) {
  if (!ReverseSort) {
    if (A.Address < B.Address)
      return true;
    else if (A.Address == B.Address && A.Name < B.Name)
      return true;
    else if (A.Address == B.Address && A.Name == B.Name && A.Size < B.Size)
      return true;
    else
      return false;
  } else {
    if (A.Address > B.Address)
      return true;
    else if (A.Address == B.Address && A.Name > B.Name)
      return true;
    else if (A.Address == B.Address && A.Name == B.Name && A.Size > B.Size)
      return true;
    else
      return false;
  }
}

static bool compareSymbolSize(const NMSymbol &A, const NMSymbol &B) {
  if (!ReverseSort) {
    if (A.Size < B.Size)
      return true;
    else if (A.Size == B.Size && A.Name < B.Name)
      return true;
    else if (A.Size == B.Size && A.Name == B.Name && A.Address < B.Address)
      return true;
    else
      return false;
  } else {
    if (A.Size > B.Size)
      return true;
    else if (A.Size == B.Size && A.Name > B.Name)
      return true;
    else if (A.Size == B.Size && A.Name == B.Name && A.Address > B.Address)
      return true;
    else
      return false;
  }
}

static bool compareSymbolName(const NMSymbol &A, const NMSymbol &B) {
  if (!ReverseSort) {
    if (A.Name < B.Name)
      return true;
    else if (A.Name == B.Name && A.Size < B.Size)
      return true;
    else if (A.Name == B.Name && A.Size == B.Size && A.Address < B.Address)
      return true;
    else
      return false;
  } else {
    if (A.Name > B.Name)
      return true;
    else if (A.Name == B.Name && A.Size > B.Size)
      return true;
    else if (A.Name == B.Name && A.Size == B.Size && A.Address > B.Address)
      return true;
    else
      return false;
  }
}

static char isSymbolList64Bit(SymbolicFile *Obj) {
  if (isa<IRObjectFile>(Obj))
    return false;
  else if (isa<COFFObjectFile>(Obj))
    return false;
  else if (MachOObjectFile *MachO = dyn_cast<MachOObjectFile>(Obj))
    return MachO->is64Bit();
  else if (isa<ELF32LEObjectFile>(Obj))
    return false;
  else if (isa<ELF64LEObjectFile>(Obj))
    return true;
  else if (isa<ELF32BEObjectFile>(Obj))
    return false;
  else if (isa<ELF64BEObjectFile>(Obj))
    return true;
  else
    return false;
}

static StringRef CurrentFilename;
typedef std::vector<NMSymbol> SymbolListT;
static SymbolListT SymbolList;

// darwinPrintSymbol() is used to print a symbol from a Mach-O file when the
// the OutputFormat is darwin or we are printing Mach-O symbols in hex.  For
// the darwin format it produces the same output as darwin's nm(1) -m output
// and when printing Mach-O symbols in hex it produces the same output as
// darwin's nm(1) -x format.
static void darwinPrintSymbol(MachOObjectFile *MachO, SymbolListT::iterator I,
                              char *SymbolAddrStr, const char *printBlanks) {
  MachO::mach_header H;
  MachO::mach_header_64 H_64;
  uint32_t Filetype, Flags;
  MachO::nlist_64 STE_64;
  MachO::nlist STE;
  uint8_t NType;
  uint8_t NSect;
  uint16_t NDesc;
  uint32_t NStrx;
  uint64_t NValue;
  if (MachO->is64Bit()) {
    H_64 = MachO->MachOObjectFile::getHeader64();
    Filetype = H_64.filetype;
    Flags = H_64.flags;
    STE_64 = MachO->getSymbol64TableEntry(I->Symb);
    NType = STE_64.n_type;
    NSect = STE_64.n_sect;
    NDesc = STE_64.n_desc;
    NStrx = STE_64.n_strx;
    NValue = STE_64.n_value;
  } else {
    H = MachO->MachOObjectFile::getHeader();
    Filetype = H.filetype;
    Flags = H.flags;
    STE = MachO->getSymbolTableEntry(I->Symb);
    NType = STE.n_type;
    NSect = STE.n_sect;
    NDesc = STE.n_desc;
    NStrx = STE.n_strx;
    NValue = STE.n_value;
  }

  // If we are printing Mach-O symbols in hex do that and return.
  if (FormatMachOasHex) {
    char Str[18] = "";
    const char *printFormat;
    if (MachO->is64Bit())
      printFormat = "%016" PRIx64;
    else
      printFormat = "%08" PRIx64;
    format(printFormat, NValue).print(Str, sizeof(Str));
    outs() << Str << ' ';
    format("%02x", NType).print(Str, sizeof(Str));
    outs() << Str << ' ';
    format("%02x", NSect).print(Str, sizeof(Str));
    outs() << Str << ' ';
    format("%04x", NDesc).print(Str, sizeof(Str));
    outs() << Str << ' ';
    format("%08x", NStrx).print(Str, sizeof(Str));
    outs() << Str << ' ';
    outs() << I->Name << "\n";
    return;
  }

  if (PrintAddress) {
    if ((NType & MachO::N_TYPE) == MachO::N_INDR)
      strcpy(SymbolAddrStr, printBlanks);
    outs() << SymbolAddrStr << ' ';
  }

  switch (NType & MachO::N_TYPE) {
  case MachO::N_UNDF:
    if (NValue != 0) {
      outs() << "(common) ";
      if (MachO::GET_COMM_ALIGN(NDesc) != 0)
        outs() << "(alignment 2^" << (int)MachO::GET_COMM_ALIGN(NDesc) << ") ";
    } else {
      if ((NType & MachO::N_TYPE) == MachO::N_PBUD)
        outs() << "(prebound ";
      else
        outs() << "(";
      if ((NDesc & MachO::REFERENCE_TYPE) ==
          MachO::REFERENCE_FLAG_UNDEFINED_LAZY)
        outs() << "undefined [lazy bound]) ";
      else if ((NDesc & MachO::REFERENCE_TYPE) ==
               MachO::REFERENCE_FLAG_UNDEFINED_LAZY)
        outs() << "undefined [private lazy bound]) ";
      else if ((NDesc & MachO::REFERENCE_TYPE) ==
               MachO::REFERENCE_FLAG_PRIVATE_UNDEFINED_NON_LAZY)
        outs() << "undefined [private]) ";
      else
        outs() << "undefined) ";
    }
    break;
  case MachO::N_ABS:
    outs() << "(absolute) ";
    break;
  case MachO::N_INDR:
    outs() << "(indirect) ";
    break;
  case MachO::N_SECT: {
    section_iterator Sec = MachO->section_end();
    MachO->getSymbolSection(I->Symb, Sec);
    DataRefImpl Ref = Sec->getRawDataRefImpl();
    StringRef SectionName;
    MachO->getSectionName(Ref, SectionName);
    StringRef SegmentName = MachO->getSectionFinalSegmentName(Ref);
    outs() << "(" << SegmentName << "," << SectionName << ") ";
    break;
  }
  default:
    outs() << "(?) ";
    break;
  }

  if (NType & MachO::N_EXT) {
    if (NDesc & MachO::REFERENCED_DYNAMICALLY)
      outs() << "[referenced dynamically] ";
    if (NType & MachO::N_PEXT) {
      if ((NDesc & MachO::N_WEAK_DEF) == MachO::N_WEAK_DEF)
        outs() << "weak private external ";
      else
        outs() << "private external ";
    } else {
      if ((NDesc & MachO::N_WEAK_REF) == MachO::N_WEAK_REF ||
          (NDesc & MachO::N_WEAK_DEF) == MachO::N_WEAK_DEF) {
        if ((NDesc & (MachO::N_WEAK_REF | MachO::N_WEAK_DEF)) ==
            (MachO::N_WEAK_REF | MachO::N_WEAK_DEF))
          outs() << "weak external automatically hidden ";
        else
          outs() << "weak external ";
      } else
        outs() << "external ";
    }
  } else {
    if (NType & MachO::N_PEXT)
      outs() << "non-external (was a private external) ";
    else
      outs() << "non-external ";
  }

  if (Filetype == MachO::MH_OBJECT &&
      (NDesc & MachO::N_NO_DEAD_STRIP) == MachO::N_NO_DEAD_STRIP)
    outs() << "[no dead strip] ";

  if (Filetype == MachO::MH_OBJECT &&
      ((NType & MachO::N_TYPE) != MachO::N_UNDF) &&
      (NDesc & MachO::N_SYMBOL_RESOLVER) == MachO::N_SYMBOL_RESOLVER)
    outs() << "[symbol resolver] ";

  if (Filetype == MachO::MH_OBJECT &&
      ((NType & MachO::N_TYPE) != MachO::N_UNDF) &&
      (NDesc & MachO::N_ALT_ENTRY) == MachO::N_ALT_ENTRY)
    outs() << "[alt entry] ";

  if ((NDesc & MachO::N_ARM_THUMB_DEF) == MachO::N_ARM_THUMB_DEF)
    outs() << "[Thumb] ";

  if ((NType & MachO::N_TYPE) == MachO::N_INDR) {
    outs() << I->Name << " (for ";
    StringRef IndirectName;
    if (MachO->getIndirectName(I->Symb, IndirectName))
      outs() << "?)";
    else
      outs() << IndirectName << ")";
  } else
    outs() << I->Name;

  if ((Flags & MachO::MH_TWOLEVEL) == MachO::MH_TWOLEVEL &&
      (((NType & MachO::N_TYPE) == MachO::N_UNDF && NValue == 0) ||
       (NType & MachO::N_TYPE) == MachO::N_PBUD)) {
    uint32_t LibraryOrdinal = MachO::GET_LIBRARY_ORDINAL(NDesc);
    if (LibraryOrdinal != 0) {
      if (LibraryOrdinal == MachO::EXECUTABLE_ORDINAL)
        outs() << " (from executable)";
      else if (LibraryOrdinal == MachO::DYNAMIC_LOOKUP_ORDINAL)
        outs() << " (dynamically looked up)";
      else {
        StringRef LibraryName;
        if (MachO->getLibraryShortNameByIndex(LibraryOrdinal - 1, LibraryName))
          outs() << " (from bad library ordinal " << LibraryOrdinal << ")";
        else
          outs() << " (from " << LibraryName << ")";
      }
    }
  }
}

  outs() << "\n";
}

// Table that maps Darwin's Mach-O stab constants to strings to allow printing.
struct DarwinStabName {
  uint8_t NType;
  const char *Name;
};
static const struct DarwinStabName DarwinStabNames[] = {
    {MachO::N_GSYM, "GSYM"},
    {MachO::N_FNAME, "FNAME"},
    {MachO::N_FUN, "FUN"},
    {MachO::N_STSYM, "STSYM"},
    {MachO::N_LCSYM, "LCSYM"},
    {MachO::N_BNSYM, "BNSYM"},
    {MachO::N_PC, "PC"},
    {MachO::N_AST, "AST"},
    {MachO::N_OPT, "OPT"},
    {MachO::N_RSYM, "RSYM"},
    {MachO::N_SLINE, "SLINE"},
    {MachO::N_ENSYM, "ENSYM"},
    {MachO::N_SSYM, "SSYM"},
    {MachO::N_SO, "SO"},
    {MachO::N_OSO, "OSO"},
    {MachO::N_LSYM, "LSYM"},
    {MachO::N_BINCL, "BINCL"},
    {MachO::N_SOL, "SOL"},
    {MachO::N_PARAMS, "PARAM"},
    {MachO::N_VERSION, "VERS"},
    {MachO::N_OLEVEL, "OLEV"},
    {MachO::N_PSYM, "PSYM"},
    {MachO::N_EINCL, "EINCL"},
    {MachO::N_ENTRY, "ENTRY"},
    {MachO::N_LBRAC, "LBRAC"},
    {MachO::N_EXCL, "EXCL"},
    {MachO::N_RBRAC, "RBRAC"},
    {MachO::N_BCOMM, "BCOMM"},
    {MachO::N_ECOMM, "ECOMM"},
    {MachO::N_ECOML, "ECOML"},
    {MachO::N_LENG, "LENG"},
    {0, 0}};
static const char *getDarwinStabString(uint8_t NType) {
  for (unsigned i = 0; DarwinStabNames[i].Name; i++) {
    if (DarwinStabNames[i].NType == NType)
      return DarwinStabNames[i].Name;
  }
  return 0;
}

// darwinPrintStab() prints the n_sect, n_desc along with a symbolic name of
// a stab n_type value in a Mach-O file.
static void darwinPrintStab(MachOObjectFile *MachO, SymbolListT::iterator I) {
  MachO::nlist_64 STE_64;
  MachO::nlist STE;
  uint8_t NType;
  uint8_t NSect;
  uint16_t NDesc;
  if (MachO->is64Bit()) {
    STE_64 = MachO->getSymbol64TableEntry(I->Symb);
    NType = STE_64.n_type;
    NSect = STE_64.n_sect;
    NDesc = STE_64.n_desc;
  } else {
    STE = MachO->getSymbolTableEntry(I->Symb);
    NType = STE.n_type;
    NSect = STE.n_sect;
    NDesc = STE.n_desc;
  }

  char Str[18] = "";
  format("%02x", NSect).print(Str, sizeof(Str));
  outs() << ' ' << Str << ' ';
  format("%04x", NDesc).print(Str, sizeof(Str));
  outs() << Str << ' ';
  if (const char *stabString = getDarwinStabString(NType))
    format("%5.5s", stabString).print(Str, sizeof(Str));
  else
    format("   %02x", NType).print(Str, sizeof(Str));
  outs() << Str;
}

static void sortAndPrintSymbolList(SymbolicFile *Obj, bool printName) {
  if (!NoSort) {
    if (NumericSort)
      std::sort(SymbolList.begin(), SymbolList.end(), compareSymbolAddress);
    else if (SizeSort)
      std::sort(SymbolList.begin(), SymbolList.end(), compareSymbolSize);
    else
      std::sort(SymbolList.begin(), SymbolList.end(), compareSymbolName);
  }

  if (OutputFormat == posix && MultipleFiles && printName) {
    outs() << '\n' << CurrentFilename << ":\n";
  } else if (OutputFormat == bsd && MultipleFiles && printName) {
    outs() << "\n" << CurrentFilename << ":\n";
  } else if (OutputFormat == sysv) {
    outs() << "\n\nSymbols from " << CurrentFilename << ":\n\n"
           << "Name                  Value   Class        Type"
           << "         Size   Line  Section\n";
  }

  const char *printBlanks, *printFormat;
  if (isSymbolList64Bit(Obj)) {
    printBlanks = "                ";
    printFormat = "%016" PRIx64;
  } else {
    printBlanks = "        ";
    printFormat = "%08" PRIx64;
  }

  for (SymbolListT::iterator I = SymbolList.begin(), E = SymbolList.end();
       I != E; ++I) {
    if ((I->TypeChar != 'U') && UndefinedOnly)
      continue;
    if ((I->TypeChar == 'U') && DefinedOnly)
      continue;
    if (SizeSort && !PrintAddress && I->Size == UnknownAddressOrSize)
      continue;
    if (JustSymbolName) {
      outs() << I->Name << "\n";
      continue;
    }

    char SymbolAddrStr[18] = "";
    char SymbolSizeStr[18] = "";

    if (OutputFormat == sysv || I->Address == UnknownAddressOrSize)
      strcpy(SymbolAddrStr, printBlanks);
    if (OutputFormat == sysv)
      strcpy(SymbolSizeStr, printBlanks);

    if (I->Address != UnknownAddressOrSize)
      format(printFormat, I->Address)
          .print(SymbolAddrStr, sizeof(SymbolAddrStr));
    if (I->Size != UnknownAddressOrSize)
      format(printFormat, I->Size).print(SymbolSizeStr, sizeof(SymbolSizeStr));

    // If OutputFormat is darwin or we are printing Mach-O symbols in hex and
    // we have a MachOObjectFile, call darwinPrintSymbol to print as darwin's
    // nm(1) -m output or hex, else if OutputFormat is darwin or we are
    // printing Mach-O symbols in hex and not a Mach-O object fall back to
    // OutputFormat bsd (see below).
    MachOObjectFile *MachO = dyn_cast<MachOObjectFile>(Obj);
    if ((OutputFormat == darwin || FormatMachOasHex) && MachO) {
      darwinPrintSymbol(MachO, I, SymbolAddrStr, printBlanks);
    } else if (OutputFormat == posix) {
      outs() << I->Name << " " << I->TypeChar << " " << SymbolAddrStr
             << SymbolSizeStr << "\n";
    } else if (OutputFormat == bsd || (OutputFormat == darwin && !MachO)) {
      if (PrintAddress)
        outs() << SymbolAddrStr << ' ';
      if (PrintSize) {
        outs() << SymbolSizeStr;
        if (I->Size != UnknownAddressOrSize)
          outs() << ' ';
      }
      outs() << I->TypeChar;
      if (I->TypeChar == '-' && MachO)
        darwinPrintStab(MachO, I);
      outs() << " " << I->Name << "\n";
    } else if (OutputFormat == sysv) {
      std::string PaddedName(I->Name);
      while (PaddedName.length() < 20)
        PaddedName += " ";
      outs() << PaddedName << "|" << SymbolAddrStr << "|   " << I->TypeChar
             << "  |                  |" << SymbolSizeStr << "|     |\n";
    }
  }

  SymbolList.clear();
}

template <class ELFT>
static char getSymbolNMTypeChar(ELFObjectFile<ELFT> &Obj,
                                basic_symbol_iterator I) {
  typedef typename ELFObjectFile<ELFT>::Elf_Sym Elf_Sym;
  typedef typename ELFObjectFile<ELFT>::Elf_Shdr Elf_Shdr;

  // OK, this is ELF
  symbol_iterator SymI(I);

  DataRefImpl Symb = I->getRawDataRefImpl();
  const Elf_Sym *ESym = Obj.getSymbol(Symb);
  const ELFFile<ELFT> &EF = *Obj.getELFFile();
  const Elf_Shdr *ESec = EF.getSection(ESym);

  if (ESec) {
    switch (ESec->sh_type) {
    case ELF::SHT_PROGBITS:
    case ELF::SHT_DYNAMIC:
      switch (ESec->sh_flags) {
      case (ELF::SHF_ALLOC | ELF::SHF_EXECINSTR):
        return 't';
      case (ELF::SHF_TLS | ELF::SHF_ALLOC | ELF::SHF_WRITE):
      case (ELF::SHF_ALLOC | ELF::SHF_WRITE):
        return 'd';
      case ELF::SHF_ALLOC:
      case (ELF::SHF_ALLOC | ELF::SHF_MERGE):
      case (ELF::SHF_ALLOC | ELF::SHF_MERGE | ELF::SHF_STRINGS):
        return 'r';
      }
      break;
    case ELF::SHT_NOBITS:
      return 'b';
    }
  }

  if (ESym->getType() == ELF::STT_SECTION) {
    StringRef Name;
    if (error(SymI->getName(Name)))
      return '?';
    return StringSwitch<char>(Name)
        .StartsWith(".debug", 'N')
        .StartsWith(".note", 'n')
        .Default('?');
  }

  return '?';
}

static char getSymbolNMTypeChar(COFFObjectFile &Obj, symbol_iterator I) {
  const coff_symbol *Symb = Obj.getCOFFSymbol(*I);
  // OK, this is COFF.
  symbol_iterator SymI(I);

  StringRef Name;
  if (error(SymI->getName(Name)))
    return '?';

  char Ret = StringSwitch<char>(Name)
                 .StartsWith(".debug", 'N')
                 .StartsWith(".sxdata", 'N')
                 .Default('?');

  if (Ret != '?')
    return Ret;

  uint32_t Characteristics = 0;
  if (!COFF::isReservedSectionNumber(Symb->SectionNumber)) {
    section_iterator SecI = Obj.section_end();
    if (error(SymI->getSection(SecI)))
      return '?';
    const coff_section *Section = Obj.getCOFFSection(*SecI);
    Characteristics = Section->Characteristics;
  }

  switch (Symb->SectionNumber) {
  case COFF::IMAGE_SYM_DEBUG:
    return 'n';
  default:
    // Check section type.
    if (Characteristics & COFF::IMAGE_SCN_CNT_CODE)
      return 't';
    else if (Characteristics & COFF::IMAGE_SCN_MEM_READ &&
             ~Characteristics & COFF::IMAGE_SCN_MEM_WRITE) // Read only.
      return 'r';
    else if (Characteristics & COFF::IMAGE_SCN_CNT_INITIALIZED_DATA)
      return 'd';
    else if (Characteristics & COFF::IMAGE_SCN_CNT_UNINITIALIZED_DATA)
      return 'b';
    else if (Characteristics & COFF::IMAGE_SCN_LNK_INFO)
      return 'i';

    // Check for section symbol.
    else if (Symb->isSectionDefinition())
      return 's';
  }

  return '?';
}

static uint8_t getNType(MachOObjectFile &Obj, DataRefImpl Symb) {
  if (Obj.is64Bit()) {
    MachO::nlist_64 STE = Obj.getSymbol64TableEntry(Symb);
    return STE.n_type;
  }
  MachO::nlist STE = Obj.getSymbolTableEntry(Symb);
  return STE.n_type;
}

static char getSymbolNMTypeChar(MachOObjectFile &Obj, basic_symbol_iterator I) {
  DataRefImpl Symb = I->getRawDataRefImpl();
  uint8_t NType = getNType(Obj, Symb);

  if (NType & MachO::N_STAB)
    return '-';

  switch (NType & MachO::N_TYPE) {
  case MachO::N_ABS:
    return 's';
  case MachO::N_INDR:
    return 'i';
  case MachO::N_SECT: {
    section_iterator Sec = Obj.section_end();
    Obj.getSymbolSection(Symb, Sec);
    DataRefImpl Ref = Sec->getRawDataRefImpl();
    StringRef SectionName;
    Obj.getSectionName(Ref, SectionName);
    StringRef SegmentName = Obj.getSectionFinalSegmentName(Ref);
    if (SegmentName == "__TEXT" && SectionName == "__text")
      return 't';
    else if (SegmentName == "__DATA" && SectionName == "__data")
      return 'd';
    else if (SegmentName == "__DATA" && SectionName == "__bss")
      return 'b';
    else
      return 's';
  }
  }

  return '?';
}

static char getSymbolNMTypeChar(const GlobalValue &GV) {
  if (GV.getType()->getElementType()->isFunctionTy())
    return 't';
  // FIXME: should we print 'b'? At the IR level we cannot be sure if this
  // will be in bss or not, but we could approximate.
  return 'd';
}

static char getSymbolNMTypeChar(IRObjectFile &Obj, basic_symbol_iterator I) {
  const GlobalValue *GV = Obj.getSymbolGV(I->getRawDataRefImpl());
  if (!GV)
    return 't';
  return getSymbolNMTypeChar(*GV);
}

template <class ELFT>
static bool isObject(ELFObjectFile<ELFT> &Obj, symbol_iterator I) {
  typedef typename ELFObjectFile<ELFT>::Elf_Sym Elf_Sym;

  DataRefImpl Symb = I->getRawDataRefImpl();
  const Elf_Sym *ESym = Obj.getSymbol(Symb);

  return ESym->getType() == ELF::STT_OBJECT;
}

static bool isObject(SymbolicFile *Obj, basic_symbol_iterator I) {
  if (ELF32LEObjectFile *ELF = dyn_cast<ELF32LEObjectFile>(Obj))
    return isObject(*ELF, I);
  if (ELF64LEObjectFile *ELF = dyn_cast<ELF64LEObjectFile>(Obj))
    return isObject(*ELF, I);
  if (ELF32BEObjectFile *ELF = dyn_cast<ELF32BEObjectFile>(Obj))
    return isObject(*ELF, I);
  if (ELF64BEObjectFile *ELF = dyn_cast<ELF64BEObjectFile>(Obj))
    return isObject(*ELF, I);
  return false;
}

static char getNMTypeChar(SymbolicFile *Obj, basic_symbol_iterator I) {
  uint32_t Symflags = I->getFlags();
  if ((Symflags & object::SymbolRef::SF_Weak) && !isa<MachOObjectFile>(Obj)) {
    char Ret = isObject(Obj, I) ? 'v' : 'w';
    if (!(Symflags & object::SymbolRef::SF_Undefined))
      Ret = toupper(Ret);
    return Ret;
  }

  if (Symflags & object::SymbolRef::SF_Undefined)
    return 'U';

  if (Symflags & object::SymbolRef::SF_Common)
    return 'C';

  char Ret = '?';
  if (Symflags & object::SymbolRef::SF_Absolute)
    Ret = 'a';
  else if (IRObjectFile *IR = dyn_cast<IRObjectFile>(Obj))
    Ret = getSymbolNMTypeChar(*IR, I);
  else if (COFFObjectFile *COFF = dyn_cast<COFFObjectFile>(Obj))
    Ret = getSymbolNMTypeChar(*COFF, I);
  else if (MachOObjectFile *MachO = dyn_cast<MachOObjectFile>(Obj))
    Ret = getSymbolNMTypeChar(*MachO, I);
  else if (ELF32LEObjectFile *ELF = dyn_cast<ELF32LEObjectFile>(Obj))
    Ret = getSymbolNMTypeChar(*ELF, I);
  else if (ELF64LEObjectFile *ELF = dyn_cast<ELF64LEObjectFile>(Obj))
    Ret = getSymbolNMTypeChar(*ELF, I);
  else if (ELF32BEObjectFile *ELF = dyn_cast<ELF32BEObjectFile>(Obj))
    Ret = getSymbolNMTypeChar(*ELF, I);
  else
    Ret = getSymbolNMTypeChar(*cast<ELF64BEObjectFile>(Obj), I);

  if (Symflags & object::SymbolRef::SF_Global)
    Ret = toupper(Ret);

  return Ret;
}

// getNsectForSegSect() is used to implement the Mach-O "-s segname sectname"
// option to dump only those symbols from that section in a Mach-O file.
// It is called once for each Mach-O file from dumpSymbolNamesFromObject()
// to get the section number for that named section from the command line
// arguments. It returns the section number for that section in the Mach-O
// file or zero it is not present.
static unsigned getNsectForSegSect(MachOObjectFile *Obj) {
  unsigned Nsect = 1;
  for (section_iterator I = Obj->section_begin(), E = Obj->section_end();
       I != E; ++I) {
    DataRefImpl Ref = I->getRawDataRefImpl();
    StringRef SectionName;
    Obj->getSectionName(Ref, SectionName);
    StringRef SegmentName = Obj->getSectionFinalSegmentName(Ref);
    if (SegmentName == SegSect[0] && SectionName == SegSect[1])
      return Nsect;
    Nsect++;
  }
  return 0;
}

// getNsectInMachO() is used to implement the Mach-O "-s segname sectname"
// option to dump only those symbols from that section in a Mach-O file.
// It is called once for each symbol in a Mach-O file from
// dumpSymbolNamesFromObject() and returns the section number for that symbol
// if it is in a section, else it returns 0.
static unsigned getNsectInMachO(MachOObjectFile &Obj, basic_symbol_iterator I) {
  DataRefImpl Symb = I->getRawDataRefImpl();
  if (Obj.is64Bit()) {
    MachO::nlist_64 STE = Obj.getSymbol64TableEntry(Symb);
    if ((STE.n_type & MachO::N_TYPE) == MachO::N_SECT)
      return STE.n_sect;
    return 0;
  }
  MachO::nlist STE = Obj.getSymbolTableEntry(Symb);
  if ((STE.n_type & MachO::N_TYPE) == MachO::N_SECT)
    return STE.n_sect;
  return 0;
}

static void dumpSymbolNamesFromObject(SymbolicFile *Obj, bool printName) {
  basic_symbol_iterator IBegin = Obj->symbol_begin();
  basic_symbol_iterator IEnd = Obj->symbol_end();
  if (DynamicSyms) {
    if (!Obj->isELF()) {
      error("File format has no dynamic symbol table", Obj->getFileName());
      return;
    }
    std::pair<symbol_iterator, symbol_iterator> IDyn =
        getELFDynamicSymbolIterators(Obj);
    IBegin = IDyn.first;
    IEnd = IDyn.second;
  }
  std::string NameBuffer;
  raw_string_ostream OS(NameBuffer);
  // If a "-s segname sectname" option was specified and this is a Mach-O
  // file get the section number for that section in this object file.
  unsigned int Nsect = 0;
  MachOObjectFile *MachO = dyn_cast<MachOObjectFile>(Obj);
  if (SegSect.size() != 0 && MachO) {
    Nsect = getNsectForSegSect(MachO);
    // If this section is not in the object file no symbols are printed.
    if (Nsect == 0)
      return;
  }
  for (basic_symbol_iterator I = IBegin; I != IEnd; ++I) {
    uint32_t SymFlags = I->getFlags();
    if (!DebugSyms && (SymFlags & SymbolRef::SF_FormatSpecific))
      continue;
    if (WithoutAliases) {
      if (IRObjectFile *IR = dyn_cast<IRObjectFile>(Obj)) {
        const GlobalValue *GV = IR->getSymbolGV(I->getRawDataRefImpl());
        if (GV && isa<GlobalAlias>(GV))
          continue;
      }
    }
    // If a "-s segname sectname" option was specified and this is a Mach-O
    // file and this section appears in this file, Nsect will be non-zero then
    // see if this symbol is a symbol from that section and if not skip it.
    if (Nsect && Nsect != getNsectInMachO(*MachO, I))
      continue;
    NMSymbol S;
    S.Size = UnknownAddressOrSize;
    S.Address = UnknownAddressOrSize;
    if ((PrintSize || SizeSort) && isa<ObjectFile>(Obj)) {
      symbol_iterator SymI = I;
      if (error(SymI->getSize(S.Size)))
        break;
    }
    if (PrintAddress && isa<ObjectFile>(Obj))
      if (error(symbol_iterator(I)->getAddress(S.Address)))
        break;
    S.TypeChar = getNMTypeChar(Obj, I);
    if (error(I->printName(OS)))
      break;
    OS << '\0';
    S.Symb = I->getRawDataRefImpl();
    SymbolList.push_back(S);
  }

  OS.flush();
  const char *P = NameBuffer.c_str();
  for (unsigned I = 0; I < SymbolList.size(); ++I) {
    SymbolList[I].Name = P;
    P += strlen(P) + 1;
  }

  CurrentFilename = Obj->getFileName();
  sortAndPrintSymbolList(Obj, printName);
}

// checkMachOAndArchFlags() checks to see if the SymbolicFile is a Mach-O file
// and if it is and there is a list of architecture flags is specified then
// check to make sure this Mach-O file is one of those architectures or all
// architectures was specificed.  If not then an error is generated and this
// routine returns false.  Else it returns true.
static bool checkMachOAndArchFlags(SymbolicFile *O, std::string &Filename) {
  if (isa<MachOObjectFile>(O) && !ArchAll && ArchFlags.size() != 0) {
    MachOObjectFile *MachO = dyn_cast<MachOObjectFile>(O);
    bool ArchFound = false;
    MachO::mach_header H;
    MachO::mach_header_64 H_64;
    Triple T;
    if (MachO->is64Bit()) {
      H_64 = MachO->MachOObjectFile::getHeader64();
      T = MachOObjectFile::getArch(H_64.cputype, H_64.cpusubtype);
    } else {
      H = MachO->MachOObjectFile::getHeader();
      T = MachOObjectFile::getArch(H.cputype, H.cpusubtype);
    }
    unsigned i;
    for (i = 0; i < ArchFlags.size(); ++i) {
      if (ArchFlags[i] == T.getArchName())
        ArchFound = true;
      break;
    }
    if (!ArchFound) {
      error(ArchFlags[i],
            "file: " + Filename + " does not contain architecture");
      return false;
    }
  }
  return true;
}

static void dumpSymbolNamesFromFile(std::string &Filename) {
  ErrorOr<std::unique_ptr<MemoryBuffer>> BufferOrErr =
      MemoryBuffer::getFileOrSTDIN(Filename);
  if (error(BufferOrErr.getError(), Filename))
    return;

  LLVMContext &Context = getGlobalContext();
  ErrorOr<Binary *> BinaryOrErr =
      createBinary(std::move(*BufferOrErr), &Context);
  if (error(BinaryOrErr.getError(), Filename))
    return;
  std::unique_ptr<Binary> Bin(BinaryOrErr.get());

  if (Archive *A = dyn_cast<Archive>(Bin.get())) {
    if (ArchiveMap) {
      Archive::symbol_iterator I = A->symbol_begin();
      Archive::symbol_iterator E = A->symbol_end();
      if (I != E) {
        outs() << "Archive map\n";
        for (; I != E; ++I) {
          ErrorOr<Archive::child_iterator> C = I->getMember();
          if (error(C.getError()))
            return;
          ErrorOr<StringRef> FileNameOrErr = C.get()->getName();
          if (error(FileNameOrErr.getError()))
            return;
          StringRef SymName = I->getName();
          outs() << SymName << " in " << FileNameOrErr.get() << "\n";
        }
        outs() << "\n";
      }
    }

    for (Archive::child_iterator I = A->child_begin(), E = A->child_end();
         I != E; ++I) {
      ErrorOr<std::unique_ptr<Binary>> ChildOrErr = I->getAsBinary(&Context);
      if (ChildOrErr.getError())
        continue;
      if (SymbolicFile *O = dyn_cast<SymbolicFile>(&*ChildOrErr.get())) {
        if (!checkMachOAndArchFlags(O, Filename))
          return;
        outs() << "\n";
        if (isa<MachOObjectFile>(O)) {
          outs() << Filename << "(" << O->getFileName() << ")";
        } else
          outs() << O->getFileName();
        outs() << ":\n";
        dumpSymbolNamesFromObject(O, false);
      }
    }
    return;
  }
  if (MachOUniversalBinary *UB = dyn_cast<MachOUniversalBinary>(Bin.get())) {
    // If we have a list of architecture flags specified dump only those.
    if (!ArchAll && ArchFlags.size() != 0) {
      // Look for a slice in the universal binary that matches each ArchFlag.
      bool ArchFound;
      for (unsigned i = 0; i < ArchFlags.size(); ++i) {
        ArchFound = false;
        for (MachOUniversalBinary::object_iterator I = UB->begin_objects(),
                                                   E = UB->end_objects();
             I != E; ++I) {
          if (ArchFlags[i] == I->getArchTypeName()) {
            ArchFound = true;
            ErrorOr<std::unique_ptr<ObjectFile>> ObjOrErr =
                I->getAsObjectFile();
            std::unique_ptr<Archive> A;
            if (ObjOrErr) {
              std::unique_ptr<ObjectFile> Obj = std::move(ObjOrErr.get());
              if (ArchFlags.size() > 1) {
                outs() << "\n" << Obj->getFileName() << " (for architecture "
                       << I->getArchTypeName() << ")"
                       << ":\n";
              }
              dumpSymbolNamesFromObject(Obj.get(), false);
            } else if (!I->getAsArchive(A)) {
              for (Archive::child_iterator AI = A->child_begin(),
                                           AE = A->child_end();
                   AI != AE; ++AI) {
                ErrorOr<std::unique_ptr<Binary>> ChildOrErr =
                    AI->getAsBinary(&Context);
                if (ChildOrErr.getError())
                  continue;
                if (SymbolicFile *O =
                        dyn_cast<SymbolicFile>(&*ChildOrErr.get())) {
                  outs() << "\n" << A->getFileName();
                  outs() << "(" << O->getFileName() << ")";
                  if (ArchFlags.size() > 1) {
                    outs() << " (for architecture " << I->getArchTypeName()
                           << ")";
                  }
                  outs() << ":\n";
                  dumpSymbolNamesFromObject(O, false);
                }
              }
            }
          }
        }
        if (!ArchFound) {
          error(ArchFlags[i],
                "file: " + Filename + " does not contain architecture");
          return;
        }
      }
      return;
    }
    // No architecture flags were specified so if this contains a slice that
    // matches the host architecture dump only that.
    if (!ArchAll) {
      StringRef HostArchName = MachOObjectFile::getHostArch().getArchName();
      for (MachOUniversalBinary::object_iterator I = UB->begin_objects(),
                                                 E = UB->end_objects();
           I != E; ++I) {
        if (HostArchName == I->getArchTypeName()) {
          ErrorOr<std::unique_ptr<ObjectFile>> ObjOrErr = I->getAsObjectFile();
          std::unique_ptr<Archive> A;
          if (ObjOrErr) {
            std::unique_ptr<ObjectFile> Obj = std::move(ObjOrErr.get());
            dumpSymbolNamesFromObject(Obj.get(), false);
          } else if (!I->getAsArchive(A)) {
            for (Archive::child_iterator AI = A->child_begin(),
                                         AE = A->child_end();
                 AI != AE; ++AI) {
              ErrorOr<std::unique_ptr<Binary>> ChildOrErr =
                  AI->getAsBinary(&Context);
              if (ChildOrErr.getError())
                continue;
              if (SymbolicFile *O =
                      dyn_cast<SymbolicFile>(&*ChildOrErr.get())) {
                outs() << "\n" << A->getFileName() << "(" << O->getFileName()
                       << ")"
                       << ":\n";
                dumpSymbolNamesFromObject(O, false);
              }
            }
          }
          return;
        }
      }
    }
    // Either all architectures have been specified or none have been specified
    // and this does not contain the host architecture so dump all the slices.
    bool moreThanOneArch = UB->getNumberOfObjects() > 1;
    for (MachOUniversalBinary::object_iterator I = UB->begin_objects(),
                                               E = UB->end_objects();
         I != E; ++I) {
      ErrorOr<std::unique_ptr<ObjectFile>> ObjOrErr = I->getAsObjectFile();
      std::unique_ptr<Archive> A;
      if (ObjOrErr) {
        std::unique_ptr<ObjectFile> Obj = std::move(ObjOrErr.get());
        if (moreThanOneArch)
          outs() << "\n";
        outs() << Obj->getFileName();
        if (isa<MachOObjectFile>(Obj.get()) && moreThanOneArch)
          outs() << " (for architecture " << I->getArchTypeName() << ")";
        outs() << ":\n";
        dumpSymbolNamesFromObject(Obj.get(), false);
      } else if (!I->getAsArchive(A)) {
        for (Archive::child_iterator AI = A->child_begin(), AE = A->child_end();
             AI != AE; ++AI) {
          ErrorOr<std::unique_ptr<Binary>> ChildOrErr =
              AI->getAsBinary(&Context);
          if (ChildOrErr.getError())
            continue;
          if (SymbolicFile *O = dyn_cast<SymbolicFile>(&*ChildOrErr.get())) {
            outs() << "\n" << A->getFileName();
            if (isa<MachOObjectFile>(O)) {
              outs() << "(" << O->getFileName() << ")";
              if (moreThanOneArch)
                outs() << " (for architecture " << I->getArchTypeName() << ")";
            } else
              outs() << ":" << O->getFileName();
            outs() << ":\n";
            dumpSymbolNamesFromObject(O, false);
          }
        }
      }
    }
    return;
  }
  if (SymbolicFile *O = dyn_cast<SymbolicFile>(Bin.get())) {
    if (!checkMachOAndArchFlags(O, Filename))
      return;
    dumpSymbolNamesFromObject(O, true);
    return;
  }
  error("unrecognizable file type", Filename);
  return;
}

int main(int argc, char **argv) {
  // Print a stack trace if we signal out.
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);

  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.
  cl::ParseCommandLineOptions(argc, argv, "llvm symbol table dumper\n");

  // llvm-nm only reads binary files.
  if (error(sys::ChangeStdinToBinary()))
    return 1;

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();

  ToolName = argv[0];
  if (BSDFormat)
    OutputFormat = bsd;
  if (POSIXFormat)
    OutputFormat = posix;
  if (DarwinFormat)
    OutputFormat = darwin;

  // The relative order of these is important. If you pass --size-sort it should
  // only print out the size. However, if you pass -S --size-sort, it should
  // print out both the size and address.
  if (SizeSort && !PrintSize)
    PrintAddress = false;
  if (OutputFormat == sysv || SizeSort)
    PrintSize = true;

  switch (InputFilenames.size()) {
  case 0:
    InputFilenames.push_back("a.out");
  case 1:
    break;
  default:
    MultipleFiles = true;
  }

  for (unsigned i = 0; i < ArchFlags.size(); ++i) {
    if (ArchFlags[i] == "all") {
      ArchAll = true;
    } else {
      Triple T = MachOObjectFile::getArch(ArchFlags[i]);
      if (T.getArch() == Triple::UnknownArch)
        error("Unknown architecture named '" + ArchFlags[i] + "'",
              "for the -arch option");
    }
  }

  if (SegSect.size() != 0 && SegSect.size() != 2)
    error("bad number of arguments (must be two arguments)",
          "for the -s option");

  std::for_each(InputFilenames.begin(), InputFilenames.end(),
                dumpSymbolNamesFromFile);

  if (HadError)
    return 1;

  return 0;
}
