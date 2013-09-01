
#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QString>
#include <QBuffer>
#include <QFileInfo>
#include <QStringList>
#include <QDebug>

static const char intro[] =
        "extern const %1 %2[] = {\n";
static const char outro[] =
        "\n};\nextern const %1 %2 = %3;";

int writeDump(const QString &srcFilename,
              const QString &dstFilename,
              const QString &dataType,
              const QString &lenType,
              const QString &dataSymbol,
              bool addNullTerm)
{
	QFile srcFile(srcFilename);
	if (!srcFile.open(QFile::ReadOnly))
		return 1;

	QFile dstFile(dstFilename);
	if (!dstFile.open(QFile::WriteOnly))
		return 1;

	QByteArray srcData = srcFile.readAll();
	if (addNullTerm)
		srcData.append('\0');

	QBuffer srcBuffer(&srcData);
	srcBuffer.open(QBuffer::ReadOnly);

	QDataStream in(&srcBuffer);
	QTextStream out(&dstFile);

	QString introStr = QString(intro).arg(dataType, dataSymbol);
	out << introStr;

	const int byteColumns = 0x10;
	int columnInd = 0;
	int byteCount = 0;

	while (!in.atEnd())
	{
		uchar byte;
		in >> byte;

		out << " ";
		if (columnInd == 0)
			out << " ";

		out << "0x";
		if (byte < 0x10)
			out << "0";
		out << QString::number(byte, 0x10);

		if (!in.atEnd())
			out << ",";

		if (++columnInd == byteColumns)
		{
			out << "\n";
			columnInd = 0;
		}

		++byteCount;
	}

	if (addNullTerm)
		--byteCount;

	QString outroStr = QString(outro).arg(lenType, dataSymbol+"_len", QString::number(byteCount));
	out << outroStr;

	return 0;
}

QString getNamedOption(const QStringList &args,
                       const QString &optName,
                       const QString &defValue)
{
	QString value = defValue;

	if (args.contains(optName))
	{
		int argInd = args.indexOf(optName);
		if (argInd < args.count())
			value = args.at(argInd+1);
	}

	return value;
}

static const char usageStr[] =
        "Usage: %1 file [options]\n"
        "Options:\n"
        "  -o [filename]     Override default output filename.\n"
        "  --symbol [sym]    Override default data C symbol.\n"
        "  --null-terminated Add null byte to data (not reflected in \"len\").\n"
        "  --string          Use char for data array. Implies \"--null-terminated\".\n"
        "  --help            Yo dawg.\n";

void usage(const QString &argv0)
{
	QTextStream out(stdout);

	out << QString(usageStr).arg(argv0);
}


int main(int argc, char *argv[])
{
	(void) argc; (void) argv;

	if (argc < 2)
	{
		usage(*argv);
		return 0;
	}

	QString inFile(argv[1]);
	QFileInfo finfo(inFile);

	QStringList restArg;
	for (int i = 2; i < argc; ++i)
		restArg << argv[i];

	if (restArg.contains("--help"))
	{
		usage(*argv);
		return 0;
	}

	bool nullTerm = false;
	bool stringData = false;

	if (restArg.contains("--null-terminated"))
		nullTerm = true;

	if (restArg.contains("--string"))
	{
		stringData = true;
		nullTerm = true;
	}

	QString outSymbol = finfo.baseName();
	outSymbol.replace(".", "_");

	QString outFile;

	outFile = getNamedOption(restArg, "-o", finfo.fileName() + ".xxd");
	outSymbol = getNamedOption(restArg, "--symbol", outSymbol);

	QString dataType = stringData ? "char" : "unsigned char";

	return writeDump(inFile, outFile,
	                 dataType, "unsigned int",
	                 outSymbol, nullTerm);
}
