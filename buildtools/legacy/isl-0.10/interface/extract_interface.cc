/*
 * Copyright 2011 Sven Verdoolaege. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY SVEN VERDOOLAEGE ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SVEN VERDOOLAEGE OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as
 * representing official policies, either expressed or implied, of
 * Sven Verdoolaege.
 */ 

#include <assert.h>
#include <iostream>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/ManagedStatic.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Basic/FileSystemOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/Version.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Tool.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/DiagnosticOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/Utils.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Sema/Sema.h>

#include "isl_config.h"
#include "extract_interface.h"
#include "python.h"

using namespace std;
using namespace clang;
using namespace clang::driver;

static llvm::cl::opt<string> InputFilename(llvm::cl::Positional,
			llvm::cl::Required, llvm::cl::desc("<input file>"));
static llvm::cl::list<string> Includes("I",
			llvm::cl::desc("Header search path"),
			llvm::cl::value_desc("path"), llvm::cl::Prefix);

static const char *ResourceDir = CLANG_PREFIX"/lib/clang/"CLANG_VERSION_STRING;

/* Does decl have an attribute of the following form?
 *
 *	__attribute__((annotate("name")))
 */
bool has_annotation(Decl *decl, const char *name)
{
	if (!decl->hasAttrs())
		return false;

	AttrVec attrs = decl->getAttrs();
	for (AttrVec::const_iterator i = attrs.begin() ; i != attrs.end(); ++i) {
		const AnnotateAttr *ann = dyn_cast<AnnotateAttr>(*i);
		if (!ann)
			continue;
		if (ann->getAnnotation().str() == name)
			return true;
	}

	return false;
}

/* Is decl marked as exported?
 */
static bool is_exported(Decl *decl)
{
	return has_annotation(decl, "isl_export");
}

/* Collect all types and functions that are annotated "isl_export"
 * in "types" and "function".
 *
 * We currently only consider single declarations.
 */
struct MyASTConsumer : public ASTConsumer {
	set<RecordDecl *> types;
	set<FunctionDecl *> functions;

	virtual HandleTopLevelDeclReturn HandleTopLevelDecl(DeclGroupRef D) {
		Decl *decl;

		if (!D.isSingleDecl())
			return HandleTopLevelDeclContinue;
		decl = D.getSingleDecl();
		if (!is_exported(decl))
			return HandleTopLevelDeclContinue;
		switch (decl->getKind()) {
		case Decl::Record:
			types.insert(cast<RecordDecl>(decl));
			break;
		case Decl::Function:
			functions.insert(cast<FunctionDecl>(decl));
			break;
		default:
			break;
		}
		return HandleTopLevelDeclContinue;
	}
};

#ifdef USE_ARRAYREF

#ifdef HAVE_CXXISPRODUCTION
static Driver *construct_driver(const char *binary, DiagnosticsEngine &Diags)
{
	return new Driver(binary, llvm::sys::getDefaultTargetTriple(),
			    "", false, false, Diags);
}
#else
static Driver *construct_driver(const char *binary, DiagnosticsEngine &Diags)
{
	return new Driver(binary, llvm::sys::getDefaultTargetTriple(),
			    "", false, Diags);
}
#endif

/* Create a CompilerInvocation object that stores the command line
 * arguments constructed by the driver.
 * The arguments are mainly useful for setting up the system include
 * paths on newer clangs and on some platforms.
 */
static CompilerInvocation *construct_invocation(const char *filename,
	DiagnosticsEngine &Diags)
{
	const char *binary = CLANG_PREFIX"/bin/clang";
	const llvm::OwningPtr<Driver> driver(construct_driver(binary, Diags));
	std::vector<const char *> Argv;
	Argv.push_back(binary);
	Argv.push_back(filename);
	const llvm::OwningPtr<Compilation> compilation(
		driver->BuildCompilation(llvm::ArrayRef<const char *>(Argv)));
	JobList &Jobs = compilation->getJobs();

	Command *cmd = cast<Command>(*Jobs.begin());
	if (strcmp(cmd->getCreator().getName(), "clang"))
		return NULL;

	const ArgStringList *args = &cmd->getArguments();

	CompilerInvocation *invocation = new CompilerInvocation;
	CompilerInvocation::CreateFromArgs(*invocation, args->data() + 1,
						args->data() + args->size(),
						Diags);
	return invocation;
}

#else

static CompilerInvocation *construct_invocation(const char *filename,
	DiagnosticsEngine &Diags)
{
	return NULL;
}

#endif

int main(int argc, char *argv[])
{
	llvm::cl::ParseCommandLineOptions(argc, argv);

	CompilerInstance *Clang = new CompilerInstance();
	DiagnosticOptions DO;
	Clang->createDiagnostics(0, NULL,
				new TextDiagnosticPrinter(llvm::errs(), DO));
	DiagnosticsEngine &Diags = Clang->getDiagnostics();
	Diags.setSuppressSystemWarnings(true);
	CompilerInvocation *invocation =
		construct_invocation(InputFilename.c_str(), Diags);
	if (invocation)
		Clang->setInvocation(invocation);
	Clang->createFileManager();
	Clang->createSourceManager(Clang->getFileManager());
	TargetOptions TO;
	TO.Triple = llvm::sys::getDefaultTargetTriple();
	TargetInfo *target = TargetInfo::CreateTargetInfo(Diags, TO);
	Clang->setTarget(target);
	CompilerInvocation::setLangDefaults(Clang->getLangOpts(), IK_C,
					    LangStandard::lang_unspecified);
	HeaderSearchOptions &HSO = Clang->getHeaderSearchOpts();
	LangOptions &LO = Clang->getLangOpts();
	PreprocessorOptions &PO = Clang->getPreprocessorOpts();
	HSO.ResourceDir = ResourceDir;

	for (int i = 0; i < Includes.size(); ++i)
		HSO.AddPath(Includes[i], frontend::Angled, true, false, false);

	PO.addMacroDef("__isl_give=__attribute__((annotate(\"isl_give\")))");
	PO.addMacroDef("__isl_keep=__attribute__((annotate(\"isl_keep\")))");
	PO.addMacroDef("__isl_take=__attribute__((annotate(\"isl_take\")))");
	PO.addMacroDef("__isl_export=__attribute__((annotate(\"isl_export\")))");
	PO.addMacroDef("__isl_constructor=__attribute__((annotate(\"isl_constructor\"))) __attribute__((annotate(\"isl_export\")))");
	PO.addMacroDef("__isl_subclass(super)=__attribute__((annotate(\"isl_subclass(\" #super \")\"))) __attribute__((annotate(\"isl_export\")))");

	Clang->createPreprocessor();
	Preprocessor &PP = Clang->getPreprocessor();

	PP.getBuiltinInfo().InitializeBuiltins(PP.getIdentifierTable(), LO);

	const FileEntry *file = Clang->getFileManager().getFile(InputFilename);
	assert(file);
	Clang->getSourceManager().createMainFileID(file);

	Clang->createASTContext();
	MyASTConsumer consumer;
	Sema *sema = new Sema(PP, Clang->getASTContext(), consumer);

	Diags.getClient()->BeginSourceFile(LO, &PP);
	ParseAST(*sema);
	Diags.getClient()->EndSourceFile();

	generate_python(consumer.types, consumer.functions);

	delete sema;
	delete Clang;
	llvm::llvm_shutdown();

	return 0;
}
