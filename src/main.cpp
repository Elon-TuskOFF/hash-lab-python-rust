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
    setWindowTitle("Hash Lab | Python + Rust");resize(800,680);
    auto *ui=new QVBoxLayout(this);
    auto *heading=new QLabel("HASH LAB");heading->setObjectName("heading");ui->addWidget(heading);
    ui->addWidget(new QLabel("OFFLINE HASH BENCHMARK   /   PYTHON + RUST"));
    auto *settings=new QGroupBox("SEARCH CONFIGURATION");ui->addWidget(settings);
    auto *form=new QFormLayout(settings);
    engine=new QComboBox;engine->addItems({"Python","Rust"});form->addRow("Engine",engine);
    hashType=new QComboBox;
    hashType->addItems({"md5","sha1","sha224","sha256","sha384","sha512",
      "sha3_224","sha3_256","sha3_384","sha3_512","blake2s","blake2b"});
    hashType->setCurrentText("blake2s");form->addRow("Hash algorithm",hashType);
    length=new QSpinBox;length->setRange(1,12);length->setValue(6);form->addRow("Length",length);
    workers=new QSpinBox;workers->setRange(1,128);workers->setValue(qMax(1,QThread::idealThreadCount()));
    form->addRow("CPU workers",workers);
    auto *charRow=new QWidget;auto *c=new QHBoxLayout(charRow);c->setContentsMargins(0,0,0,0);
    lower=new QCheckBox("a-z");upper=new QCheckBox("A-Z");
    numbers=new QCheckBox("0-9");symbols=new QCheckBox("!@#$%^&*_-");
    lower->setChecked(true);symbols->setChecked(true);
    for(auto *cb:{lower,upper,numbers,symbols})c->addWidget(cb);
    form->addRow("Character sets",charRow);
    digest=new QLineEdit("713417394e2b74aee7fa54d09375b0c78d288a90eba27a560cfc698c5b54886c");
    form->addRow("Target hash",digest);
    auto *actions=new QHBoxLayout;
    auto *preset=new QPushButton("Variant 5 preset");
    start=new QPushButton("▶ Start");start->setObjectName("primary");
    stop=new QPushButton("Stop");stop->setEnabled(false);
    actions->addWidget(preset);actions->addStretch();actions->addWidget(stop);actions->addWidget(start);
    form->addRow(actions);
    auto *monitor=new QGroupBox("LIVE PROGRESS");ui->addWidget(monitor);
    auto *v=new QVBoxLayout(monitor);
    status=new QLabel("Ready.");v->addWidget(status);
    bar=new QProgressBar;bar->setRange(0,10000);bar->setValue(0);v->addWidget(bar);
    info=new QLabel("Candidates: —     Speed: —     Estimated remaining: —");
    info->setWordWrap(true);v->addWidget(info);
    answer=new QLabel("Password: —");answer->setObjectName("answer");v->addWidget(answer);
    ui->addStretch();
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
       cancelled=true;status->setText("Stopping...");
       if(bench)process.kill();else process.write("STOP\n");
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
           answer->setText("Password: "+(parts[1].isEmpty()?"Not found":parts[1]));
           status->setText("Finished.");
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
       QWidget {background:#0d1424;color:#eef2ff;font:10pt 'Segoe UI';}
       QLabel#heading {font-size:28pt;font-weight:bold;color:#8db0ff;}
       QLabel#answer {font:14pt Consolas;color:#75e6c3;}
       QGroupBox {background:#172239;border:1px solid #344664;border-radius:14px;
          margin-top:15px;padding:18px;font-weight:bold;}
       QGroupBox::title {subcontrol-origin:margin;left:16px;top:2px;}
       QLineEdit,QSpinBox,QComboBox {background:#0f192e;border:1px solid #435679;
          color:white;border-radius:7px;padding:8px;}
       QComboBox QAbstractItemView {background:#1d2940;color:white;}
       QPushButton {background:#2a3d5a;color:white;padding:10px;border-radius:8px;}
       QPushButton#primary {background:#4c78ed;font-weight:bold;}
       QPushButton:disabled {color:#7a8394;background:#1f283b;}
       QProgressBar {background:#111c30;border:0;border-radius:7px;height:18px;}
       QProgressBar::chunk {background:#5b85ff;border-radius:7px;}
    )");
  }
};
#include "main.moc"
int main(int argc,char** argv){QApplication app(argc,argv);Window w;w.show();return app.exec();}
