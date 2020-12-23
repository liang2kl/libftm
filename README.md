# WIFI FTM Tool

*当前版本存在问题，请使用 [d002f7f](https://github.com/liang2kl/wifi-ftm/tree/d002f7f1f4c730242552f93c784de28c19b210ea)*

## 环境配置

### 系统要求

需要 linux 内核版本为 `5.8+`

### 安装 libnl 库

1. 安装依赖

```
sudo apt install bison flex make gcc
```

2. 安装 libnl

```
wget https://github.com/thom311/libnl/releases/download/libnl3_5_0/libnl-3.5.0.tar.gz
tar -xzf libnl-3.5.0.tar.gz
cd libnl-3.5.0
./configure --prefix=/usr     \
            --sysconfdir=/etc \
            --disable-static  &&
make
sudo make install
```

3. 配置 libnl

```
sudo rm /lib/x86_64-linux-gnu/libnl-3.so.200.26.0
sudo rm /lib/x86_64-linux-gnu/libnl-genl-3.so.200.26.0
```

## 使用

### 克隆主分支

```
git clone https://github.com/liang2kl/wifi-ftm.git main
```

### 编译与安装

在项目根目录（`wifi-ftm`）下，执行：

```
make
```

```
sudo make install
```

### 运行

#### 作为 initiator

```
sudo ftm start_measurement <接口名称> <配置文件路径> [<次数>]
```

#### 作为 responder

```
sudo ftm start_responder <接口名称>
```

## 其他

其他文档：[Wiki](https://github.com/liang2kl/wifi-ftm/wiki)

调用 API 的示例程序：[demo 分支](https://github.com/liang2kl/wifi-ftm/tree/demo)

调用 API 的示例程序（配置文件）：[config-file-demo 分支](https://github.com/liang2kl/wifi-ftm/tree/config-file-demo)
