const std = @import("std");

pub fn build(b: *std.Build) void {
    const exe = b.addExecutable(.{
        .name = "c2-client",
        .target = b.standardTargetOptions(.{}),
        .optimize = b.standardOptimizeOption(.{}),
    });

    exe.linkLibC();
    exe.addCSourceFiles(.{
        .root = b.path("src"),
        .files = &.{
            "main.c",
            "command.c",
            "socket_utils.c",
        },
    });

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the client app.");
    run_step.dependOn(&run_cmd.step);
}
