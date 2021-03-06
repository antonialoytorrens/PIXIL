#include <pwd.h>
#include <unistd.h>
#include <qapp.h>
#include <X11/Xlib.h>
#include <qbitmap.h>
#include "main.h"  // ssApp
#include <kapp.h>  // for kde_bindir()
#include <kcharsets.h>
#include <kprocess.h>
#include <qpushbt.h>
#include <qlined.h>
#include <qframe.h>
#include <qkeycode.h>
//#include "config.h"
#include "saver.h"

#include "saver.moc"

#define MAX_PASSWORD_LENGTH	20

extern KLocale *glocale;
extern ssApp *globalKapp;

int checkPasswd(char *);


kScreenSaver::kScreenSaver( Drawable drawable ) : QObject()
{
	Window root;
	int ai;
	unsigned int au;

	d = drawable;
	gc = XCreateGC( qt_xdisplay(), d, 0, 0);
	XGetGeometry( qt_xdisplay(), drawable, &root, &ai, &ai,
		&width, &height, &au, &au ); 
}

kScreenSaver::~kScreenSaver()
{
	XFreeGC( qt_xdisplay(), gc );
}

void kScreenSaver::expose( int x, int y, int width, int height )
{
    XSetForeground(qt_xdisplay(), gc, BlackPixel(qt_xdisplay(), qt_xscreen()));
    XFillRectangle(qt_xdisplay(), d, gc, x, y, width, height);                  
}

//-----------------------------------------------------------------------------

KPasswordDlg::KPasswordDlg( QWidget *parent, bool s ) : QWidget( parent )
{
	setCursor( arrowCursor );

	stars = s;
	password = "";

	QFrame *frame = new QFrame( this );
	frame->setFrameStyle( QFrame::Panel | QFrame::Raised );
	frame->setLineWidth( 2 );
	frame->setGeometry( 0, 0, 200, 100 );

	QFont font( "helvetica", 18 );
	KApplication::getKApplication()->getCharsets()->setQFont(font);
	label = new QLabel( glocale->translate("Enter Password"), frame );
	label->setGeometry( 20, 20, 160, 30 );
	label->setAlignment( AlignCenter );
	label->setFont( font );
	
	font.setPointSize( 16 );
	KApplication::getKApplication()->getCharsets()->setQFont(font);
	entry = new QLabel( "", frame );
	entry->setGeometry( 20, 60, 160, 30 );
	entry->setFont( font );	

	resize( 200, 100 );

	connect( &timer, SIGNAL( timeout() ), SLOT( timeout() ) );

	timerMode = 0;
	timer.start( 10000, TRUE );
	
	if( stars )
	{
		blinkTimer = new QTimer( this, "blink" );
		connect( blinkTimer, SIGNAL( timeout() ), SLOT( blinkTimeout() ) );
		blinkTimer->start( 300 );
		blink = false;
	}
}

void KPasswordDlg::showStars()
{
	QString s;
	
	s.fill( '*', password.length() );
	if( blink )
		s += "_";
		
	entry->setText( s );	
}

void KPasswordDlg::keyPressed( QKeyEvent *e )
{
  static bool waitForAuthentication = false;
  if (!waitForAuthentication) {
	switch ( e->key() )
	{
		case Key_Backspace:
			{
				int len = password.length();
				if ( len ) {
					password.truncate( len - 1 );
					if( stars )
						showStars();
				}
			}
			break;

		case Key_Return:
			waitForAuthentication = true;
			if ( tryPassword() )
				emit passOk();
			else
			{
				label->setText( glocale->translate("Failed") );
				password = "";
				timerMode = 1;
				timer.start( 1500, TRUE );
			}
			waitForAuthentication = false;
			break;

		case Key_Escape:
			emit passCancel();
			break;

		default:
			if ( password.length() < MAX_PASSWORD_LENGTH )
			{
				password += (char)e->ascii();
				if( stars )
					showStars();
				timer.changeInterval( 10000 );
			}
	}
  }
}


int KPasswordDlg::tryPassword()
{
#if defined HAVE_ETCPASSWD || defined HAVE_SHADOW || defined HAVE_PAM
	if( stars )
	  blinkTimer->stop();
	KProcess chkpass;
	QString kcp_binName = "";
	kcp_binName += KApplication::kde_bindir();
	kcp_binName += "/kcheckpass";
	chkpass.clearArguments();
	chkpass << kcp_binName;
	bool ret = chkpass.start(KProcess::DontCare, KProcess::Stdin);
	if (ret == false) {
	  if( stars )
	    blinkTimer->start( 300 );
	  return 2;
	}
        chkpass.writeStdin(password.data(),password.length()); // write Password to stdin
	chkpass.closeStdin();                // eof

	int timeout = 1000;
	while ( timeout != 0 ) {
	  if (! chkpass.isRunning() )
	    break;
	  else {
	    globalKapp->processEvents();
	    timeout--;
	    usleep(10000);
	  }
	}
	
	int rc = ( chkpass.normalExit() && (chkpass.exitStatus() == 0) );
	if( stars )
	  blinkTimer->start( 300 ); 
	return rc;
#else
	int e = checkPasswd(password.data());
	return e;
#endif
}

void KPasswordDlg::timeout()
{
	if ( timerMode )
	{
		label->setText( glocale->translate("Enter Password") );
		if( stars )
			showStars();
		timerMode = 0;
		timer.start( 5000, TRUE );
	}
	else
		emit passCancel();
}

void KPasswordDlg::blinkTimeout()
{
	blink = !blink;
	showStars();
}

