#include <QtWidgets>
class Window : public QWidget {
  Q_OBJECT
  QComboBox *engine,*hashType;
  QSpinBox *length,*workers;
  QCheckBox *lower,*upper,*numbers,*symbols;
  QLineEdit *digest;
  QPushButton *start,*stop;
  QLabel *status,*info,*answer;
  QProgressBar *bar;
  QProcess process;
  QByteArray pending;
  QString root, chars, alg, target;
  double total=1;
  bool bench=false, cancelled=false;
  qint64 stoppingPid=0;
  void run(bool benchmark){
    bench=benchmark;
    QString program;
    QStringList args={"--chars",chars,"--length",QString::number(length->value()),
       "--algorithm",alg,"--workers",QString::number(workers->value())};
    if(engine->currentText()=="Python"){
#ifdef Q_OS_WIN
      program=QFileInfo::exists(root+"/runtime/python/python.exe") ? root+"/runtime/python/python.exe" : "python";
#else
      program="python3";
#endif
      args.prepend(root+"/backend/python/engine.py");args.prepend("-u");
    } else {
#ifdef Q_OS_WIN
      program=QFileInfo::exists(root+"/runtime/rust/hash_lab_rust.exe") ? root+"/runtime/rust/hash_lab_rust.exe" : root+"/backend/rust/target/release/hash_lab_rust.exe";
#else
      program=root+"/backend/rust/target/release/hash_lab_rust";
#endif
    }
    if(bench)args<<"--benchmark";else args<<"--target"<<target;
    stoppingPid=0;
    pending.clear();process.setWorkingDirectory(root);process.start(program,args);
  }
public:
  Window(){
    QDir dir(QCoreApplication::applicationDirPath());
    for(int i=0;i<5&&!QFileInfo::exists(dir.filePath("backend/python/engine.py"));++i)dir.cdUp();
    root=dir.absolutePath();
    if (!QFileInfo::exists(root+"/backend/python/engine.py")) {
      QDir working(QDir::currentPath());
      for(int i=0;i<5&&!QFileInfo::exists(working.filePath("backend/python/engine.py"));++i)working.cdUp();
      if(QFileInfo::exists(working.filePath("backend/python/engine.py"))) root=working.absolutePath();
    }
    setWindowIcon(QIcon(QCoreApplication::applicationDirPath()+"/hashlab.ico"));
    setWindowTitle("Hash Lab | Python + Rust");resize(970,930);setMinimumSize(790,730);
    auto *windowLayout=new QVBoxLayout(this);
    windowLayout->setContentsMargins(0,0,0,0);
    auto *scroll=new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *content=new QWidget;
    content->setObjectName("content");
    auto *ui=new QVBoxLayout(content);
    ui->setContentsMargins(34,30,34,30);
    ui->setSpacing(0);
    auto *heading=new QLabel("HASH LAB");heading->setObjectName("heading");ui->addWidget(heading);
    ui->addSpacing(14);
    auto *subtitle=new QLabel("OFFLINE HASH BENCHMARK   /   PYTHON + RUST");
    subtitle->setObjectName("subtitle");ui->addWidget(subtitle);
    ui->addSpacing(34);
    auto *settings=new QGroupBox("SEARCH CONFIGURATION");ui->addWidget(settings);
    auto *form=new QFormLayout(settings);
    settings->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    form->setContentsMargins(26,30,26,25);
    form->setVerticalSpacing(16);
    form->setHorizontalSpacing(26);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto addField=[&](const QString &text,QWidget *control){
      auto *label=new QLabel(text); label->setObjectName("formLabel");
      label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      label->setMinimumHeight(46);
      form->addRow(label,control);
    };
    auto counter=[&](QSpinBox *spin)->QWidget* {
      spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
      spin->setAlignment(Qt::AlignCenter);
      spin->setFixedHeight(46);
      auto *wrapper=new QWidget; wrapper->setObjectName("counterWrap");
      wrapper->setFixedHeight(48);
      wrapper->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
      auto *line=new QHBoxLayout(wrapper);
      line->setSpacing(8); line->setContentsMargins(0,0,0,0);
      auto *minus=new QPushButton("−"); auto *plus=new QPushButton("+");
      minus->setObjectName("counterButton"); plus->setObjectName("counterButton");
      for (auto *button:{minus,plus}) {
        button->setFixedSize(46,46);
        button->setCursor(Qt::PointingHandCursor);
        button->setAutoRepeat(true);
        button->setAutoRepeatDelay(350);
        button->setAutoRepeatInterval(75);
      }
      QObject::connect(minus,&QPushButton::clicked,spin,[spin]{spin->stepDown();});
      QObject::connect(plus,&QPushButton::clicked,spin,[spin]{spin->stepUp();});
      QObject::connect(spin,QOverload<int>::of(&QSpinBox::valueChanged),wrapper,[=](int value){
        minus->setEnabled(value>spin->minimum());
        plus->setEnabled(value<spin->maximum());
      });
      minus->setEnabled(spin->value()>spin->minimum());
      plus->setEnabled(spin->value()<spin->maximum());
      line->addWidget(minus); line->addWidget(spin,1); line->addWidget(plus);
      return wrapper;
    };
    engine=new QComboBox;engine->addItems({"Python","Rust"});addField("Engine",engine);
    hashType=new QComboBox;
    hashType->addItems({"md5","sha1","sha224","sha256","sha384","sha512",
      "sha3_224","sha3_256","sha3_384","sha3_512","blake2s","blake2b"});
    hashType->setCurrentText("blake2s");addField("Hash algorithm",hashType);
    length=new QSpinBox;length->setRange(1,12);length->setValue(6);addField("Length",counter(length));
    workers=new QSpinBox;workers->setRange(1,128);workers->setValue(qMax(1,QThread::idealThreadCount()));
    addField("CPU workers",counter(workers));
    auto *charRow=new QWidget;charRow->setObjectName("charRow");
    charRow->setMinimumHeight(76);
    charRow->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    auto *c=new QHBoxLayout(charRow);c->setContentsMargins(0,19,0,19);c->setSpacing(32);
    lower=new QCheckBox("a-z");upper=new QCheckBox("A-Z");
    numbers=new QCheckBox("0-9");symbols=new QCheckBox("!@#$%^&*_-");
    lower->setChecked(true);symbols->setChecked(true);
    for(auto *cb:{lower,upper,numbers,symbols}) {cb->setMinimumHeight(34);c->addWidget(cb);}
    c->addStretch();
    addField("Character sets",charRow);
    digest=new QLineEdit("713417394e2b74aee7fa54d09375b0c78d288a90eba27a560cfc698c5b54886c");
    addField("Target hash",digest);
    auto *actions=new QHBoxLayout;actions->setSpacing(12);
    auto *preset=new QPushButton("↺  Variant 5 preset");preset->setObjectName("secondary");
    start=new QPushButton("▶  Start search");start->setObjectName("primary");
    stop=new QPushButton("■  Stop");stop->setObjectName("danger");stop->setEnabled(false);
    for(auto *button : {preset,start,stop}) {button->setMinimumHeight(46);button->setCursor(Qt::PointingHandCursor);}
    start->setMinimumWidth(152);stop->setMinimumWidth(95);
    actions->addWidget(preset);actions->addStretch();actions->addWidget(stop);actions->addWidget(start);
    form->addRow(actions);
    ui->addSpacing(26);
    auto *monitor=new QGroupBox("LIVE PROGRESS");ui->addWidget(monitor);
    auto *v=new QVBoxLayout(monitor);
    v->setContentsMargins(26,32,26,24);v->setSpacing(18);
    status=new QLabel("Ready to start a local search.");status->setObjectName("status");v->addWidget(status);
    bar=new QProgressBar;bar->setRange(0,10000);bar->setValue(0);v->addWidget(bar);
    info=new QLabel("Candidates: —     Speed: —     Estimated remaining: —");
    info->setObjectName("information");info->setWordWrap(true);v->addWidget(info);
    answer=new QLabel("Password: —");answer->setObjectName("answer");v->addWidget(answer);
    ui->addStretch();
    scroll->setWidget(content);
    windowLayout->addWidget(scroll);
    connect(preset,&QPushButton::clicked,this,[this]{
       lower->setChecked(true);upper->setChecked(false);numbers->setChecked(false);
       symbols->setChecked(true);length->setValue(6);hashType->setCurrentText("blake2s");
       digest->setText("713417394e2b74aee7fa54d09375b0c78d288a90eba27a560cfc698c5b54886c");
    });
    connect(start,&QPushButton::clicked,this,[this]{
       chars.clear();
       if(lower->isChecked())chars+="abcdefghijklmnopqrstuvwxyz";
       if(upper->isChecked())chars+="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
       if(numbers->isChecked())chars+="0123456789";
       if(symbols->isChecked())chars+="!@#$%^&*_-";
       if(chars.isEmpty()){QMessageBox::warning(this,"Error","Choose characters");return;}
       alg=hashType->currentText();target=digest->text().trimmed().toLower();
       QMap<QString,int> sizes={{"md5",32},{"sha1",40},{"sha224",56},{"sha256",64},
          {"sha384",96},{"sha512",128},{"sha3_224",56},{"sha3_256",64},
          {"sha3_384",96},{"sha3_512",128},{"blake2s",64},{"blake2b",128}};
       if(target.size()!=sizes[alg] || !QRegularExpression("^[0-9a-f]+$").match(target).hasMatch()){
         QMessageBox::warning(this,"Error","Invalid digest for this algorithm");return;
       }
       total=std::pow(double(chars.size()),length->value());cancelled=false;
       start->setEnabled(false);stop->setEnabled(true);bar->setValue(0);
       answer->setText("Password: —");status->setText("Benchmarking...");
       run(true);
    });
    connect(stop,&QPushButton::clicked,this,[this]{
       if(cancelled)return;
       cancelled=true;stop->setEnabled(false);
       status->setText("Stopping workers...");
       if(process.state()==QProcess::NotRunning){return;}
       const qint64 pid=process.processId();
       stoppingPid=pid;
       // First request cooperative cancellation. Escalate if the workers do not exit.
       if(bench)process.kill();
       else {process.write("STOP\n");process.waitForBytesWritten(100);}
       QTimer::singleShot(1800,this,[this,pid]{
         if(!cancelled || process.state()==QProcess::NotRunning || process.processId()!=pid)return;
#ifdef Q_OS_WIN
         // Python multiprocessing leaves child workers alive when only the parent is killed.
         QProcess::startDetached("taskkill", {"/PID",QString::number(pid),"/T","/F"});
#else
         process.kill();
#endif
         QTimer::singleShot(1300,this,[this,pid]{
           if(process.state()!=QProcess::NotRunning && process.processId()==pid)process.kill();
         });
       });
    });
    connect(&process,&QProcess::readyReadStandardOutput,this,[this]{
       pending+=process.readAllStandardOutput();
       int pos;
       while((pos=pending.indexOf('\n'))>=0){
         QStringList parts=QString::fromUtf8(pending.left(pos)).trimmed().split('\t');
         pending.remove(0,pos+1);
         if(parts[0]=="RATE"&&parts.size()>1){
           double r=parts[1].toDouble()*workers->value();
           info->setText("Keyspace: "+QString::number(total,'f',0)+
                         "  |  Estimated worst case: "+QString::number(total/qMax(1.0,r),'f',1)+" seconds");
         }else if(parts[0]=="PROGRESS"&&parts.size()>2){
           double n=parts[1].toDouble(),t=parts[2].toDouble();
           bar->setValue(int(qBound(0.0,n/total,1.0)*10000));
           double r=n/qMax(0.01,t);
           info->setText("Checked "+QString::number(n,'f',0)+" / "+QString::number(total,'f',0)+
              "  |  "+QString::number(r,'f',0)+" hashes/s  |  Remaining ~"+
              QString::number((total-n)/qMax(1.0,r),'f',1)+" sec");
         }else if(parts[0]=="DONE"&&parts.size()>4){
           if(!cancelled){
             answer->setText("Password: "+(parts[1].isEmpty()?"Not found":parts[1]));
             status->setText("Finished.");
           }
         }else if(parts[0]=="ERROR")status->setText(parts.join(" "));
       }
    });
    connect(&process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError e){
       if(e==QProcess::FailedToStart){
         status->setText("Backend missing. Install Python or build Rust.");
         start->setEnabled(true);stop->setEnabled(false);
       }
    });
    connect(&process,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
      this,[this](int code,QProcess::ExitStatus exit){
        if(bench&&!cancelled&&code==0&&exit==QProcess::NormalExit){
           status->setText("Searching...");run(false);
        }else{
           if(cancelled)status->setText("Stopped.");
           else if(code!=0)status->setText("Backend error: "+QString::fromUtf8(process.readAllStandardError()));
           start->setEnabled(true);stop->setEnabled(false);
        }
    });
    setStyleSheet(R"(
       QWidget {background:#08090C;color:#E8E9ED;font:10pt 'Segoe UI';}
       QWidget#counterWrap, QWidget#charRow {background:transparent;}
       QScrollArea {background:#08090C;border:none;}
       QLabel {background:transparent;border:none;}
       QLabel#formLabel {color:#D0D2D9;font-weight:500;padding:0;}
       QLabel#heading {font-size:31pt;font-weight:800;color:#FAFAFC;letter-spacing:2px;}
       QLabel#subtitle {font-size:10pt;color:#858B97;letter-spacing:1px;}
       QLabel#status {font-size:10pt;color:#ADB9D5;font-weight:600;}
       QLabel#information {font-size:10pt;color:#BFC4CF;}
       QLabel#answer {font:14pt Consolas;color:#85E3BA;}
       QGroupBox {background:#101216;border:1px solid #272A32;border-radius:16px;
          margin-top:18px;padding:0;font-weight:700;color:#E9EAF0;}
       QGroupBox::title {subcontrol-origin:margin;left:19px;top:0px;padding:0 10px;
          background:#08090C;color:#DADCE4;font-size:10pt;}
       QLineEdit,QSpinBox,QComboBox {background:#191C22;border:1px solid #343841;
          color:#F5F6F8;border-radius:9px;padding:10px 13px;min-height:23px;
          selection-background-color:#587AF2;}
       QLineEdit:hover,QSpinBox:hover,QComboBox:hover {border:1px solid #555B67;}
       QLineEdit:focus,QSpinBox:focus,QComboBox:focus {border:1px solid #7894F9;}
       QComboBox QAbstractItemView {background:#1B1D23;color:#F5F6F8;
          selection-background-color:#384B84;padding:5px;}
       QCheckBox {background:transparent;spacing:9px;color:#CED1DA;}
       QCheckBox::indicator {width:17px;height:17px;}
       QPushButton {background:#242830;border:1px solid #383D48;color:#ECEEF4;
          padding:11px 20px;border-radius:10px;font-weight:700;}
       QPushButton:hover {background:#303641;border-color:#566076;}
       QPushButton:pressed {background:#1A1D24;}
       QPushButton#counterButton {min-width:46px;min-height:46px;max-width:46px;max-height:46px;background:#252930;border:1px solid #3C414B;
          border-radius:10px;color:#E6E8EE;padding:0;font-size:18pt;font-weight:600;}
       QPushButton#counterButton:hover {background:#373F4A;border-color:#78869D;}
       QPushButton#counterButton:pressed {background:#4F5D76;}
       QPushButton#counterButton:disabled {color:#666B76;background:#1B1D22;border-color:#292D33;}
       QPushButton#primary {background:#6D88F8;border:1px solid #7894FA;color:#0A0E19;}
       QPushButton#primary:hover {background:#8FA6FF;border-color:#A0B4FF;}
       QPushButton#primary:pressed {background:#526EE2;}
       QPushButton#secondary {background:#202329;border:1px solid #373C46;color:#DDE0E9;}
       QPushButton#secondary:hover {background:#30343E;border-color:#505764;}
       QPushButton#danger {background:#2B2025;border:1px solid #62414A;color:#F4B3C1;}
       QPushButton#danger:hover {background:#452931;border-color:#965968;}
       QPushButton:disabled {background:#1B1D22;color:#676B75;border:1px solid #292D33;}
       QProgressBar {background:#202329;border:0;border-radius:8px;height:19px;
          text-align:center;color:#E5EAF5;font-weight:600;}
       QProgressBar::chunk {background:#738CF4;border-radius:8px;}
    )");
  }
};
#include "main.moc"
int main(int argc,char** argv){QApplication app(argc,argv);Window w;w.show();return app.exec();}
