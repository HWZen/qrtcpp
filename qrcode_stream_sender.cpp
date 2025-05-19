#include "qrcodegen.hpp"
#include "wirehair.h"

#include <print>
#include <vector>
#include <iostream>
#include <sstream>
#include <random>

#include <QApplication>
#include <QMainWindow>
#include <QSvgWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QByteArray>
#include <QTimer>
#include <QLabel>

using std::vector, std::cout, std::endl;
using namespace qrcodegen;


static std::string toSvgString(const QrCode &qr, int border)
{
    if (border < 0)
        throw std::domain_error("Border must be non-negative");
    if (border > INT_MAX / 2 || border * 2 > INT_MAX - qr.getSize())
        throw std::overflow_error("Border too large");

    std::ostringstream sb;
    sb << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    sb << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    sb << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" viewBox=\"0 0 ";
    sb << (qr.getSize() + border * 2) << " " << (qr.getSize() + border * 2) << "\" stroke=\"none\">\n";
    sb << "\t<rect width=\"100%\" height=\"100%\" fill=\"#FFFFFF\"/>\n";
    sb << "\t<path d=\"";
    for (int y = 0; y < qr.getSize(); y++)
    {
        for (int x = 0; x < qr.getSize(); x++)
        {
            if (qr.getModule(x, y))
            {
                if (x != 0 || y != 0)
                    sb << " ";
                sb << "M" << (x + border) << "," << (y + border) << "h1v1h-1z";
            }
        }
    }
    sb << "\" fill=\"#000000\"/>\n";
    sb << "</svg>\n";
    return sb.str();
}

class QRCodeWindow : public QMainWindow {
    Q_OBJECT
public:
    QRCodeWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("QR Code Viewer");
        resize(1000, 1100);
        
        centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        layout = new QVBoxLayout(centralWidget);
        
        svgWidget = new QSvgWidget(centralWidget);
        layout->addWidget(svgWidget);
        
        statusLabel = new QLabel(centralWidget);
        layout->addWidget(statusLabel);

        layout->setStretch(0, 10);  // svgWidget占10份
        layout->setStretch(1, 1);   // statusLabel占1份
        
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &QRCodeWindow::updateQRCode);
    }
    
    void startDisplay(const vector<uint8_t>& _message, int _packetSize) {
        this->message = _message;
        this->packetSize = _packetSize;
        currentBlockId = 0;
        
        encoder = wirehair_encoder_create(nullptr, &message[0], message.size(), packetSize);
        if (!encoder) {
            statusLabel->setText("Failed to create encoder");
            return;
        }
        
        timer->start(50); // 刷新率
    }
    
private slots:
    void updateQRCode() {
        if (!encoder) return;



        // +0: blockId
        // +4: data total size
        // +8: block size
        // +12: block data
        vector<uint8_t> block(packetSize + 4 + 4 + 4);

        auto *blockPtr = (uint32_t*)&block[0];
        *blockPtr = currentBlockId;
        uint32_t writeLen = 0;

        auto *dataTotalSizePtr = (uint32_t*)&block[4];
        *dataTotalSizePtr = (uint32_t)message.size();

        auto *blockSizePtr = (uint32_t*)&block[8];
        *blockSizePtr = packetSize;
        
        WirehairResult encodeResult = wirehair_encode(
            encoder,
            currentBlockId,
            &block[4 + 4 + 4],
            packetSize,
            &writeLen
        );
        
        if (encodeResult != Wirehair_Success) {
            statusLabel->setText(QString("Encode failed at block %1").arg(currentBlockId));
            timer->stop();
            return;
        }
        block.resize(writeLen + 4 + 4 + 4);

        // 将block编码为base64
        QByteArray qarrayBlock((const char *)block.data(), (int)block.size());
        auto base64block = qarrayBlock.toBase64();
        auto vecBase64block = std::vector<uint8_t>(base64block.begin(), base64block.end());

        // 创建二维码
        // QrCode qr = QrCode::encodeBinary(block, QrCode::Ecc::LOW);
        QrCode qr = QrCode::encodeBinary(vecBase64block, QrCode::Ecc::LOW);
        
        // 转换为SVG
        auto svg = toSvgString(qr, 10);
        
        // 更新显示
        QByteArray svgData(svg.c_str());
        svgWidget->load(svgData);
        
        // 更新状态
        statusLabel->setText(QString("Block ID: %1, Size: %2 bytes").arg(currentBlockId).arg(vecBase64block.size()));
        
        currentBlockId++;
    }
    
private:
    QWidget *centralWidget;
    QVBoxLayout *layout;
    QSvgWidget *svgWidget;
    QLabel *statusLabel;
    QTimer *timer;
    
    vector<uint8_t> message;
    int packetSize;
    unsigned currentBlockId;
    WirehairCodec encoder;
};

#include "qrcode_stream_sender.moc"

extern std::string send_message;

int main(int argc, char *argv[]) try
{
    const WirehairResult initResult = wirehair_init();

    if (initResult != Wirehair_Success)
    {
        cout << "!!! Wirehair initialization failed: " << initResult << endl;
        return -1;
    }
    //
    // if (!ReadmeExample())
    // {
    //     cout << "!!! Example usage failed" << endl;
    //     return -2;
    // }
    QApplication app(argc, argv);
    
    // 准备测试数据
    constexpr int kPacketSize = 600;
    constexpr int kMessageBytes = 1024 * 50;

//     std::string send_message = R"(
// Triton 旨在通过提供一种可以编译为高效的 CUDA 本地代码的高级抽象，使编写高性能 GPU 代码变得更加容易。在这篇文章中，我将深入探讨 Triton 的内部机制，并探索 Triton 程序如何在幕后编译为 CUDA 内核（具体来说是 CUBIN）Cuda Compilation在深入了解 triton 之前，了解使用 nvcc 的 cuda 编译过程是有用的。以下来自 NVIDIA 文档的图表展示了整个 cuda 编译为可执行代码的过程。
// CUDA 编译器（nvcc）预处理输入程序以供设备编译。这包括处理如#include 和与设备代码相关的宏等指令。 预处理后的设备代码被编译成 CUDA 二进制文件（cubin）和/或 PTX（并行线程执行）中间代码。 PTX 是 CUDA 的低级虚拟机和汇编语言。它允许进行优化并且是架构无关的。 Cubin 是针对目标架构优化的 GPU 特定二进制代码。两者 cubin 和 PTX 都会被放入 fatbinary 中，fatbinary 包含针对不同 GPU 架构优化的多种代码版本。看一个简单的 cuda -> ptx 的例子。
//
// 作者：Arthur
// 链接：https://zhuanlan.zhihu.com/p/1900918897022046521
// 来源：知乎
// 著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。
// )";


    while (send_message.size() < kMessageBytes)
    {
        send_message += send_message;
    }

    vector<uint8_t> message(send_message.begin(), send_message.end());
    message.resize(kMessageBytes);
    
    // 创建并显示窗口
    QRCodeWindow window;
    window.show();
    
    // 开始显示二维码
    window.startDisplay(message, kPacketSize);
    
    return app.exec();
}
catch (std::exception &e)
{
    std::println(stderr, "Exception: {}", e.what());
    return -1;
}


std::string send_message = R"(
C++的异常处理一直以来都是争议巨大的话题，有人认为异常处理好，有些人认为不好，甚至极其讨厌c++异常处理、把项目中禁用异常处理作为c++开发原则之一。

而我，我个人是观点是尽量不使用c++异常。

当我在接触过java的异常处理以及尝试在自己的线程库封装pthread_cancel时，更加坚定了我的观点。



一、异常处理的优缺点
抛开c++不谈，异常处理还是有很多值得肯定的地方的，这点也有大量其它文章介绍，我简单总结一下：

异常不能不加修改地忽略
异常处理能使正常代码与错误处理的代码分离，使代码清晰
用异常处理的方式处理构造函数、操作符重载中产生的错误更优雅
异常处理的缺点，其实也不是很致命、不可接受，无非就是：

异常处理机制性能较差
代码可读性变差


总的来说，我一开始学习异常处理的时候，感觉是“嗯，这种try ...catch...的错误处理方式，挺不错的呀~”

但是我后来发现，c++的异常处理就是一个巨坑！



二、为什么不要用C++的异常处理
一言蔽之，就是：

想要在C++项目中实现异常安全是极其困难的

我有幸在上个学年学习过一个学期java，学的不多，写得不多，也就用tomcat这个老古董做了个网页聊天平台。

开发下来，java给我的感觉就是一门非常无聊的语言：感觉面对一个需求/任务，10个人能写出一模一样的代码。我认为这是它的缺点，也是它的优点：非常稳定，非常适合写业务。

为什么我会提到java？因为我非常喜欢java的异常处理，我认为它是有用的、能提高程序稳定性和有助于维护的。

下面讲讲java异常处理中一些我非常喜欢的、且c++没有的特性，并从中论证我的观点。

1.抛出异常是Java统一的错误处理方式
不管是什么错误，java都是以抛出异常的方式处理的，不管是用户自己的逻辑错误、还是框架的错误、还是系统级别的错误，统统都是抛异常。

C++的错误处理就非常割裂

先来讲讲简单的用户层面，有两种主流错误处理方式：抛异常和错误码，java或许也有错误码的错误处理方式，但至少不算主流。

当你的项目用了两个库，一个用异常处理，一个用错误码，你的心智负担将会急剧提升。

到系统层面、语言层面，还有一个很特别的错误处理方式：中断信号

最经典的：除数为0导致的错误，引发SIGFPE信号；

最常见的：被滥用的abort()，引发SIGABRT信号;

还有SIGTERM，有没有必须释放的资源，或是希望在程序死掉前保留些什么信息；

其它还有各种各样的信号，每个操作系统都各有不同，而且每个信号的默认处理方式也不一样。

这就意味着，假设要为一个c++项目做异常安全设计，可能要考虑：

抛出的异常有没有妥善处理
错误码有没有全部检查
中断信号有没有考虑周全
再加上c++的手动gc，那心智负担，属实令人头大。



其实我还是蛮喜欢signal信号处理机制的，它和异常相比，有一个非常明显的优点，下面会谈到。

2.Java的异常处理机制比c++的成熟非常多
这点首先体现在java异常处理对用户的约束上：

强制性异常

如果你想要抛出一个强制性异常，那么你必须在函数头声明会抛出什么异常；同时，如果你使用某个函数，你必须显示处理函数声明会抛出的异常，或者继续向上声明，你的函数会抛出什么什么异常。

我觉得这是非常非常非常重要的一个设计，这个设计使得方法作者和方法调用者确保自己有遵守异常规范，同时也利于ide进行异常分析，在最人性化的IDE——IntelliJ的智能补全下，Java程序员想要实现异常安全是非常简单的事。最重要的是，它保证了Java有一个良好的异常安全的“生态”。

而c++，它完全没有考虑过函数调用者该如何了解函数会抛出什么异常，它也完全没为函数作者考虑过，用的人不知道，或者不处理他抛出的异常该怎么办。



强制性的异常类型约束

throw的对象必须是Throwable的子类，我觉得这是非常正确且天经地义的设计。

但是c++却没有这么做，c++虽然有std::exception，但是不会限制你throw什么东西。

我觉得有两个原因：一是c++提倡自由，二是面对c++的std::exception，可能有人不满意它的性能；可能有人不喜欢它的设计；可能有人就是喜欢自己造轮子，做一个自己的异常基类。

但不管怎么说，对于使用者而言，都是极不友好的，java如果不想管异常，直接catch(Throwable), 再打印一下异常类型、堆栈信息就好了，但对于c++，虽然你能catch(...)，但是你完全catch了个寂寞，和java相比就差太远了。



非常方便实用的printStackTrace方法

虽然和c++一样，在catch的时候try里面的堆栈已经释放掉了，但是java还是能够输出最基本的抛出异常时的调用堆栈，方便开发者定位异常引发位置。

而c++就非常尴尬了：你正在使用一个库，你突然发现使用的库在抛异常，但是你并不知道是哪里会抛异常，于是你加了一个全局范围的try catch，你捕获到了这个异常，但是你除了把它的what()打印出来，什么都干不了，没有堆栈信息，what()里面也没有足够有效的信息，你还是无法定位到底是哪里抛出了异常。除非不处理异常，引发core dump，在转储文件中找堆栈。

相比之下，信号中断处理反而是没有释放资源，保留了堆栈，使得你完全可以在中断处理函数中做好资源处理，再反手fork一个子进程，来一个core dump，既做好了资源管理，也留下了转储文件，美哉美哉。



3.Java的异常“生态”非常好
我学java不过寥寥几月，接触到的框架不多，也就一点tomcat和spring-boot。

而我对java框架的其中一个印象，就是它们的异常处理做得非常好，非常的安全。tomcat服务中的某个连接实例出现问题，抛出异常，我总能非常方便的知道原因，以及快速定位到位置。服务也不会轻易崩溃，总能很好的捕获和处理异常。

而且我也没听Java程序员抱怨过一个项目在使用多个框架、多个库的情况下异常不好处理的问题。

而c++的话，某些系统级别的库, 比如UE4、ROS，会做好自己的异常安全处理。但如果要在项目中引入多个这样的库，比如写一个基于Qt+UE4+OSG+Ros的机器人AR分析工具，那么就非常考验开发者的解耦能力，以及异常处理能力了。

再加上整个c++社区，禁用异常、使用错误码的项目、组织不在小数，你一个人/库做好了异常安全，也不能保证整个程序的异常安全，所以也有不少库完全不管异常，出问题整个程序直接掐掉，毕竟有可能做和不做程序都是不安全的。



三、GCC的pthread
前一阵子，我为自己的stl添加了thread库，总的来说开发过程还是顺利的，毕竟不管是Linux还是Windows，都有提供比较完善的线程相关的接口。

开发大概两三周，其中大部分时间都花在调试研究Linux平台的线程退出问题

傻逼GCC竟然通过抛出异常的方式来结束线程！！！纯纯的脑瘫行为！！！

简单说明一下GCC对pthread_cancel的实现：

当线程要意外终止时，程序会让线程抛出一个异常，这个异常一路抛出，不被处理，以此达到在终止线程的同时，进行资源回收的目的。

我是不知道GNU那帮人是怎么想的，但这搭配上pthread_cancel的异步终止方式，绝对是灾难！

因为你很难保证这个异常能够顺利抛出。

如果你想要使用pthread_cancel终止一个设定为pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr); 的线程，你最好保证：

线程的启动的函数不是noexcept的。
线程终止时，整个线程调用堆栈中，没有nothrow函数
线程终止时，整个线程调用堆栈中，没有形如try{}catch(...){} 的无差别捕获语句（但可以有try{}catch(...){ throw;}语句）
有人说，这看起来也没什么，注意一下就好了。

但是你们忽略了一类经常使用的、且在现实中大概率会存在于线程终止时的调用堆栈中的函数：会阻塞的C库函数.

在现实中，线程运行的大部分的时间可能并不是在你的业务逻辑上，而是在各种各样的io阻塞上，比如等待socket，等待stdio，等待条件变量，等待sleep……

而这些函数都是C库函数，都是没有异常处理的，假如线程被这些函数阻塞，又用pthread_cancel将其停止，将有很大概率触发terminate called without an active exception abort.

总的来说，除非万不得已，不要用g++的pthread_cancel, 如果真要用，可以考虑开启禁用异常，这将会有效减少你的心智负担。

四、C++异常处理痛点总结
对于函数编写者：

你只能通过注释或者提供使用文档的方式告诉使用者，你的函数会抛出什么异常。
你无法保证抛出的异常是否真的能被使用者接收到，因为你的函数可能运行于一个noexcept函数中，或者你的函数被编译成动态库，并开启了-fno-exceptions选项
对于函数使用者：

你完全无法确定这个函数会不会抛异常、会抛什么异常（哪怕函数作者自己没有写抛异常，函数也有可能被pthread_cancel掉从而抛异常）
当你捕获到异常，你可能完全不知道这个异常是从哪里抛出的，因为完全没有堆栈信息
对于所有c++开发者：

c++错误处理方式混乱，只做好异常安全远远不够，完全不能保持程序的安全。
c++异常处理机制落后，假设一个程序不是每一行代码、每一个函数，都由一个开发者从头到尾开发的，那么他完全无法保证他的合作者能够遵守异常规范。
做好c++的异常安全，心智负担太大。
五、C++异常处理可以改进的点
以下，是我基于现代c++，对c++异常处理提出的一些改进建议

1.位于函数头的强制性的异常声明，以及强制性的异常处理
这个必须有！这是保证c++异常生态能够推广的必不可少的特性！

不是像c++98那样的，可有可无的异常规范，而是像Java那样的强制性的异常声明，不声明就不能通过编译的那种，这样看似是更严格，但是开发者反而能够通过编译器错误提示得知自己应该声明哪些异常、处理哪些异常，也有助于IDE进行异常方面的提示以及智能补全。

我觉得这是更有利于实现异常安全的。

没有这个根本不会有人用c++异常。

2. 所有抛出的异常都应该继承自一个异常基类
设一个std::exception_base虚基类，并取消try{}catch(...){}这种捕获方式。



2. 将try...catch...协程化
协程是c++20最重要的特性之一，我认为可以将异常处理协程化，具体做法是把catch部分作为一个协程的入口，当try部分有异常抛出，就可以启动符合的catch协程，当catch处理完后，再释放try部分的资源。

这样做，就能在catch的时候保留抛出异常的堆栈了，在抛出异常的时候，我就能在catch里打一个断点，了解到是哪里抛出了异常，以及追踪完整的堆栈信息，这个是连java异常都做不到的功能。

六. 后记
虽然我上面提了一些c++异常处理可以改进的点，但是我还是不看好c++的异常处理。

这种机制根本就不适合c++。

因为c++太过自由，同时涉及的领域又极其广泛。

异常处理或许适合业务开发，当你需要做兼容层，混用c++和c时、需要进行性能限制十分严苛的嵌入式开发时，异常处理根本无法满足要求。



我是一名c++爱好者，从高二开始接触c++，至今已有五个年头，期间我也接触过其他语言: python, matlab, kotlin, java.

它们对比c++，或许有更合理的设计、更短的学习周期，有比c++多很多的优点。

但是，没有语言敢说比c++更强大。

c++下跟操作系统打交道，中间有oop，上有普通人无法驾驭的模板元编程。

我只能说，c++是真正的魔法，令我着迷的魔法。

虽然人们对c++委员会的工作效率很不满意，但c++11/14/17，到如今的c++20，以及即将到来的c++23，我是见到c++在往一个正确的方向不断进步的，尤其是c++20，它代表着c++不是一门古老的、应该被淘汰的语言，它是来自2020年的、现代化的、超前的语言，它代表着c++仍有活力、仍在不断进化！

祝c++越来越好！

)";