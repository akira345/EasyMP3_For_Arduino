#include <SD.h>

File root;
//EasyMP3
//
//参考：ＶＳ１００１Ｋの使い方
//     
//http://www.yuki-lab.jp/hw/mp3player.html
//
unsigned char beep_start[]={ 
  0x53, 0xEF, 0x6E, 0x31, 0, 0, 0, 0 };
unsigned char beep_stop[]={ 
  0x45, 0x78, 0x69, 0x74, 0, 0, 0, 0 };
//EasyMP3 ピン定義
#define SI 2
#define SCLK 3
#define CS 4
#define RESET 5
#define BSYNC 6
#define DCLK 7
#define DREQ 8

void setup(){

  pinMode(SI,OUTPUT);
  pinMode(SCLK,OUTPUT);
  pinMode(CS,OUTPUT);
  pinMode(RESET,OUTPUT);
  pinMode(BSYNC,INPUT);
  pinMode(DCLK,OUTPUT);
  pinMode(DREQ,INPUT);
  //microSDカードの接続は、SDカートとピンアサインが違うので注意
  //http://yoshi-s.cocolog-nifty.com/cpu/avr_cpm/index.html
  //接続は以下のサイトと同じです。(microSDかSDカードかの違いだけ)
  //http://arms22.blog91.fc2.com/blog-entry-294.html
  pinMode(0,INPUT);
  digitalWrite(0,LOW);
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  //SD Card Initialize
  pinMode(10, OUTPUT);
  if (!SD.begin(10)) {
    //initialization failed!
    return;
  }
  // initialization done.


  //EasyMP3初期化処理
  //ハードウエアリセット解除
  digitalWrite(RESET,HIGH);
  //1msec以上待つ
  delay(1);
  //EasyMP3初期化
  MP3_init();
  
  unsigned char i;
   
   //Beepを鳴らしてみる
   
   for (i=0;i<8;++i){
   // Serial.write(beep_start[i]);
   MP3_data_write(beep_start[i]);
   }
   delay(500);
   for (i=0;i<8;++i){
   MP3_data_write(beep_stop[i]);
   }
   delay(500);
   //
   
  root = SD.open("/");
  //play_mp3(root);

  MP3_Play(root);





}
//****************************************
//MP3ファイル再生
//****************************************
void MP3_Play(File dir) {
  while(true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      //Serial.println("**nomorefiles**");
      break;
    }

    Serial.print(entry.name());
    if (entry.isDirectory()) {
         Serial.println("/");
         //再帰呼び出しはメモリ的に厳しかったのでやめる
     // MP3_Play(entry);
    } 
    else {
      // files have sizes, directories do not

      File file = SD.open(entry.name(), FILE_READ);
      if(file){

        char buf[512];
        unsigned long sz;
        while((sz = file.read((uint8_t*)buf, sizeof(buf))) > 0){
          for(unsigned long i=0; i<sz; i++){
            if ( MP3_data_write(buf[i]) == false){
              break;
            }
          }
        }

        file.close();
      }
      else{
        //   Serial.println("can not open.");
      }

      digitalWrite(BSYNC,HIGH); 
      digitalWrite(BSYNC,LOW);
      //速度的には余り関係がなかった
      //*out_bsync |= bit_bsync;//High
      //*out_bsync &= ~bit_bsync;//Low 
      for (unsigned long i=0;i<2048;++i){
        MP3_data_write(0);
      }
       digitalWrite(BSYNC,HIGH); 
      //*out_bsync |= bit_bsync;//High

      MP3_init();

      //   Serial.print("\t\t");
      //   Serial.println(entry.size(), DEC);
    }
  }
}
//****************************************
//MP3データByte送信
//****************************************
bool MP3_data_write(unsigned char data){
  //DREQがHighに成るまで待つ
    if(digitalRead(DREQ) == LOW){
  //なにか処理を入れる。
   }else{
  //BSYNCをHIGH
   digitalWrite(BSYNC,HIGH);
  //*out_bsync |= bit_bsync;//High
  //1Byte送信
    new_shiftOut(SI,DCLK,MSBFIRST,data);
  //BSYNCをLow
    digitalWrite(BSYNC,LOW); 
  //*out_bsync &= ~bit_bsync;//Low
  return true;
    }
}
//****************************************
//	EasyMP3 初期化
//****************************************
void MP3_init(void){
  //DREQがHighに成るまで待つ
  while(digitalRead(DREQ) == LOW);
  //ソフトウエアリセット
  MP3_command(0, 0x0004);
  //5usec以上まつ
  delayMicroseconds(5);
  //解除
  MP3_command(0, 0x0000);
  //DREQがHighに成るまで待つ
  while(digitalRead(DREQ) == LOW);
  //クロック指定(14.318Mhz)
  MP3_command(3, 0x8000 + (unsigned int)((14318)/2));


  //ボリューム指定
  Set_Vol(0x2F);

}
//****************************************
//	ボリュームセット（左右同時）
//****************************************
void Set_Vol(unsigned int data){
  //ボリューム指定 上位8bit=左 下位8bit=右
  MP3_command(11, (data << 8)|(data));	// 音量設定 
}
//****************************************
//	EasyMP3コマンド送信
//****************************************
void MP3_command(unsigned char addr,unsigned int arg){
  //CSをLowにする
  digitalWrite(CS,LOW);
  MP3_command_write(2);//書き込みモード
  MP3_command_write(addr);//アドレス指定
  MP3_command_write(arg >> 8);//上位8bit
  MP3_command_write(arg);//下位8bit
  //CSをHighにする
  digitalWrite(CS,HIGH);
}
//****************************************
//	EasyMP3 コマンド 1byte出力
//****************************************
void MP3_command_write(unsigned char data){
   new_shiftOut(SI,SCLK,MSBFIRST,data);
   digitalWrite(SI,HIGH);
}

//****************************************
//http://arms22.blog91.fc2.com/blog-entry-213.html
//より高速化したShiftOut
//****************************************
void new_shiftOut(uint8_t dataPin,
uint8_t clockPin,
uint8_t bitOrder,
byte val)
{
  int i;
  uint8_t bit_data = digitalPinToBitMask(dataPin);
  uint8_t bit_clock = digitalPinToBitMask(clockPin);
  volatile uint8_t *out_data = portOutputRegister(digitalPinToPort(dataPin));
  volatile uint8_t *out_clock = portOutputRegister(digitalPinToPort(clockPin));

  for (i = 0; i < 8; i++)  {
    if (bitOrder == LSBFIRST){
      if(val & (1 << i))
        *out_data |= bit_data;
      else
        *out_data &= ~bit_data;
    }
    else{
      if(val & (1 << (7 - i))){
        *out_data |= bit_data;
      }
      else{
        *out_data &= ~bit_data;
      }
    }

    *out_clock |= bit_clock;
    *out_clock &= ~bit_clock;
  }
}



void loop(){

}



