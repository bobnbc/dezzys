<?php

// autoload_static.php @generated by Composer

namespace Composer\Autoload;

class ComposerStaticInit376bfbed2f48f665768f0d59f5e80ff5
{
    public static $prefixesPsr0 = array (
        'P' => 
        array (
            'Payeezy_' => 
            array (
                0 => __DIR__ . '/..' . '/payeezy/payeezy-php/src',
            ),
        ),
    );

    public static function getInitializer(ClassLoader $loader)
    {
        return \Closure::bind(function () use ($loader) {
            $loader->prefixesPsr0 = ComposerStaticInit376bfbed2f48f665768f0d59f5e80ff5::$prefixesPsr0;

        }, null, ClassLoader::class);
    }
}
