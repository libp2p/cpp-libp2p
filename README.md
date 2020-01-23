# CPP-Libp2p  

> Fully compatible C++17 implementation of libp2p library

Libp2p is a modular networking stack described in [spec](https://github.com/libp2p/specs)

## Dependencies

All dependencies are managed using [Hunter](hunter.sh). It uses cmake to download required libraries and does not require downloading and installing packages manually.
Target C++ compilers are:
* GCC 7.4
* Clang 6.0.1
* AppleClang 11.0

## Supported protocols
* Transports: TCP
* Security protocols: [Plaintext 2.0](https://github.com/libp2p/specs/blob/master/plaintext/README.md), [SECIO](https://github.com/libp2p/specs/blob/master/secio/README.md)
* Multiplexing protocols: [MPlex](https://github.com/libp2p/specs/tree/master/mplex), [Yamux](https://github.com/hashicorp/yamux/blob/master/spec.md)
* [Kademlia DHT](https://github.com/libp2p/specs/pull/108)
* [Gossipsub](https://github.com/libp2p/specs/tree/master/pubsub/gossipsub) (WIP)
* [Identify](https://github.com/libp2p/specs/tree/master/identify)

## Development
### Clone

To clone repository execute
```
git clone https://github.com/soramitsu/libp2p.git
```

### Build cpp-libp2p

First build will likely take long time. However, you can cache binaries to [hunter-binary-cache](https://github.com/soramitsu/hunter-binary-cache) or even download binaries from the cache in case someone has already compiled project with the same compiler. To do so you need to set up two environment variables:
```
GITHUB_HUNTER_USERNAME=<github account name>
GITHUB_HUNTER_TOKEN=<github token>
```
To generate github token follow the [instructions](https://help.github.com/en/github/authenticating-to-github/creating-a-personal-access-token-for-the-command-line). Make sure `read:packages` and `write:packages` permissions are granted (step 7 in instructions).

This project can be built with

```
mkdir build && cd build
cmake -DCLANG_TIDY=ON ..
make -j
```

It is suggested to build project with clang-tidy checks, however if you wish to omit clang-tidy step, you can use `cmake ..` instead.

Tests can be run with: 
```
cd build
ctest
```

### CodeStyle

We follow [CppCoreGuidelines](https://github.com/isocpp/CppCoreGuidelines).

Please use provided [.clang-format](.clang-format) file to autoformat the code.

## Examples

Please explore [example](example) section to read examples of how to use the library

## Adding cpp-libp2p to the project

cpp-libp2p can be integrated using hunter. Adding hunter support to your project is really simple and require you only to add some cmake. Check [hunter example project](https://github.com/forexample/hunter-simple/) for details. 

After hunter is integrated adding cpp-libp2p can be done by adding these lines to cmake:
```cmake
hunter_add_package(libp2p)
find_package(libp2p REQUIRED)
``` 
To set which version of cpp-libp2p to use it is required to add these lines to Hunter/config.cmake file:
```cmake
hunter_config(libp2p
    URL https://github.com/soramitsu/libp2p/archive/dad84a03a9651c7c2bb8a8f17d0e5ea67bd10b4f.zip
    SHA1 860742c6e3e9736d68b392513d795e09572780aa
    )
``` 
Where URL is the link to archive of certain commit in cpp-libp2p repo and SHA1 is the checksum of this archive.
By simply updating URL and SHA1 it is possible to change the version of cpp-libp2p in another project. 

Example of adding cpp-libp2p to other project can be found [here](https://github.com/soramitsu/kagome/blob/3edda60f27d378a21fc57cd8bec7f0f519203318/cmake/dependencies.cmake#L59) and [here](https://github.com/soramitsu/kagome/blob/3edda60f27d378a21fc57cd8bec7f0f519203318/cmake/Hunter/config.cmake#L24)

## Notable users

* https://github.com/soramitsu/kagome
* https://github.com/filecoin-project/cpp-filecoin  

## Maintenance

Maintainers: [@Warchant], [@kamilsa], [@harrm], [@masterjedy], [@igor-egorov], [@art-gor]

[@Warchant]: https://github.com/Warchant 
[@kamilsa]: https://github.com/kamilsa 
[@harrm]: https://github.com/harrm 
[@masterjedy]: https://github.com/masterjedy 
[@igor-egorov]: https://github.com/igor-egorov 
[@art-gor]: https://github.com/art-gor 
    
Tickets: Can be opened in Github Issues.
