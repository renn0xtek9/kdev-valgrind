#include <qwhatsthis.h>
#include <qregexp.h>
#include <qfile.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kgenericfactory.h>
#include <kaction.h>
#include <kprocess.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kdebug.h>

#include "kdevcore.h"
#include "kdevmainwindow.h"
#include "kdevproject.h"

#include "valgrind_widget.h"
#include "valgrind_part.h"
#include "valgrind_dialog.h"
#include "valgrinditem.h"

typedef KGenericFactory<ValgrindPart> ValgrindFactory;
K_EXPORT_COMPONENT_FACTORY( libkdevvalgrind, ValgrindFactory( "kdevvalgrind" ) );

ValgrindPart::ValgrindPart( QObject *parent, const char *name, const QStringList& )
  : KDevPlugin( "Valgrind", "valgrind", parent, name ? name : "ValgrindPart" )
{
  setInstance( ValgrindFactory::instance() );
  setXMLFile( "kdevpart_valgrind.rc" );

  proc = new KShellProcess();
  connect( proc, SIGNAL(receivedStdout( KProcess*, char*, int )),
           this, SLOT(receivedStdout( KProcess*, char*, int )) );
  connect( proc, SIGNAL(receivedStderr( KProcess*, char*, int )),
           this, SLOT(receivedStderr( KProcess*, char*, int )) );
  connect( proc, SIGNAL(processExited( KProcess* )),
           this, SLOT(processExited( KProcess* )) );
  connect( core(), SIGNAL(stopButtonClicked(KDevPlugin*)),
           this, SLOT(slotStopButtonClicked(KDevPlugin*)) );
  
  m_widget = new ValgrindWidget( this );
  
  QWhatsThis::add( m_widget, i18n( "Valgrind memory leak check" ) );

  new KAction( i18n("&Valgrind Memory Leak Check"), 0, this,
	       SLOT(slotExecValgrind()), actionCollection(), "tools_valgrind" );
  
  mainWindow()->embedOutputView( m_widget, "Valgrind", "Valgrind memory leak check" );
}


ValgrindPart::~ValgrindPart()
{
  delete m_widget;
  delete proc;
}

void ValgrindPart::loadOutput()
{
  QString fName = KFileDialog::getOpenFileName(QString::null, "*", 0, i18n("Open Valgrind Output"));
  if ( fName.isEmpty() )
    return;

  QFile f( fName );
  if ( !f.open( IO_ReadOnly ) ) {
    KMessageBox::sorry( 0, i18n("Could not open valgrind output: %1").arg(fName) );
    return;
  }
  
  clear();
  getActiveFiles();

  QTextStream stream( &f );
  while ( !stream.atEnd() ) {
    receivedString( stream.readLine() + "\n" );
  }
  f.close();
}

void ValgrindPart::getActiveFiles()
{
  activeFiles.clear();
  if ( project() ) {
    QStringList projectFiles = project()->allFiles();
    QString projectDirectory = project()->projectDirectory();
    KURL url;
    for ( QStringList::Iterator it = projectFiles.begin(); it != projectFiles.end(); ++it ) {
      KURL url( projectDirectory + "/" + (*it) );
      url.cleanPath( true );
      activeFiles += url.path();
      kdDebug() << "set project file: " << url.path().latin1() << endl;
    }
  }
}

static void guessActiveItem( ValgrindItem& item, const QStringList activeFiles )
{
  if ( activeFiles.isEmpty() && item.backtrace().isEmpty() )
    return;
  for ( ValgrindItem::BacktraceList::Iterator it = item.backtrace().begin(); it != item.backtrace().end(); ++it ) {
    // active: first line of backtrace that lies in project source file
    for ( QStringList::ConstIterator it2 = activeFiles.begin(); it2 != activeFiles.end(); ++it2 ) {
      if ( (*it).url() == (*it2) ) {
        (*it).setHighlighted( true );
        return;
      }
    }
  }
}

void ValgrindPart::appendMessage( const QString& message )
{
  if ( message.isEmpty() )
    return;

  ValgrindItem item( message );
  guessActiveItem( item, activeFiles );
  m_widget->addMessage( item );
}

void ValgrindPart::slotExecValgrind()
{
  ValgrindDialog* dlg = new ValgrindDialog();
  if ( project() && _lastExec.isEmpty() ) {
    dlg->setExecutable( project()->mainProgram() );
  } else {
    dlg->setExecutable( _lastExec );
  }
  dlg->setParameters( _lastParams );
  dlg->setValExecutable( _lastValExec );
  dlg->setValParams( _lastValParams );
  if ( dlg->exec() == QDialog::Accepted ) {
    runValgrind( dlg->executableName(), dlg->parameters(), dlg->valExecutable(), dlg->valParams() );
  }
}

void ValgrindPart::slotKillValgrind()
{
  if ( proc )
    proc->kill();
}

void ValgrindPart::slotStopButtonClicked( KDevPlugin* which )
{
  if ( which != 0 && which != this )
    return;
  slotKillValgrind();
}

void ValgrindPart::clear()
{
  m_widget->clear();
  currentMessage = QString::null;
  currentPid = -1;
  lastPiece = QString::null;
}

void ValgrindPart::runValgrind( const QString& exec, const QString& params, const QString& valExec, const QString& valParams )
{
  if ( proc->isRunning() ) {
    KMessageBox::sorry( 0, i18n( "There is already an instance of valgrind running." ) );
    return;
    // todo - ask for forced kill
  }
  
  getActiveFiles();
  
  proc->clearArguments();  
  *proc << valExec << valParams << exec << params;
  proc->start( KProcess::NotifyOnExit, KProcess::AllOutput );
  mainWindow()->raiseView( m_widget );
  core()->running( this, true );

  _lastExec = exec;
  _lastParams = params;
  _lastValExec = valExec;
  _lastValParams = valParams;
}

void ValgrindPart::receivedStdout( KProcess*, char* /* msg */, int /* len */ )
{
  //kdDebug() << "got StdOut: " <<QString::fromLocal8Bit( msg, len ) << endl;
}

void ValgrindPart::receivedStderr( KProcess*, char* msg, int len )
{
  receivedString( QString::fromLocal8Bit( msg, len ) );
}

void ValgrindPart::receivedString( const QString& str )
{
  QString rmsg = lastPiece + str;
  QStringList lines = QStringList::split( "\n", rmsg );

//  kdDebug() << "got: " << QString::fromLocal8Bit( msg, len ) << endl;

  if ( !rmsg.endsWith( "\n" ) ) {
    // the last message is trucated, we'll receive
    // the rest in the next call
    lastPiece = lines.back();
    lines.pop_back();
  } else {
    lastPiece = QString::null;
  }
  appendMessages( lines );
}

void ValgrindPart::appendMessages( const QStringList& lines )
{
  QRegExp valRe( "==(\\d+)== (.*)" );

  for ( QStringList::ConstIterator it = lines.begin(); it != lines.end(); ++it ) {
    if ( valRe.search( *it ) < 0 )
      continue;

    int cPid = valRe.cap( 1 ).toInt();

    if ( valRe.cap( 2 ).isEmpty() ) {
      appendMessage( currentMessage );
      currentMessage = QString::null;
    } else if ( cPid != currentPid ) {
      appendMessage( currentMessage );
      currentMessage = *it;
      currentPid = cPid;
    } else {
      if ( !currentMessage.isEmpty() )
        currentMessage += "\n";
      currentMessage += *it;
    }
  }
}

void ValgrindPart::processExited( KProcess* p )
{
  if ( p == proc ) {
    appendMessage( currentMessage + lastPiece );
    currentMessage = QString::null;
    lastPiece = QString::null;
    core()->running( this, false );
  }
}

void ValgrindPart::restorePartialProjectSession( const QDomElement* el )
{
  QDomElement execElem = el->namedItem( "executable" ).toElement();
  _lastExec = execElem.attribute( "path", "" );
  _lastParams = execElem.attribute( "params", "" );

  QDomElement valElem = el->namedItem( "valgrind" ).toElement();
  _lastValExec = valElem.attribute( "path", "" );
  _lastValParams = valElem.attribute( "params", "" );
}

void ValgrindPart::savePartialProjectSession( QDomElement* el )
{
  QDomDocument domDoc = el->ownerDocument();
  if ( domDoc.isNull() ) 
    return;

  QDomElement execElem = domDoc.createElement( "executable" );
  execElem.setAttribute( "path", _lastExec );
  execElem.setAttribute( "params", _lastParams );

  QDomElement valElem = domDoc.createElement( "valgrind" );
  valElem.setAttribute( "path", _lastValExec );
  valElem.setAttribute( "params", _lastValParams );

  el->appendChild( execElem );
  el->appendChild( valElem );
}

#include "valgrind_part.moc"
